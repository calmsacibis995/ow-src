
#ifndef lint
static char sccsid[] = "@(#)rast.c 1.33 96/06/18";
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

#include <pixrect/rasterfile.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"

#define  ESCAPE   128
#define  PIX_ERR  -1
#define  MAXDEPTH 32

FILE 		*save_file;
int		 pad_info [MAXDEPTH];

extern int	 ispopen;

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
	for(i=0; i<4; i++)
	    tmp[3-i]=*(bp+i);
	for(i=0; i<4; i++)
	    *(bp++) = tmp[i];
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
 
int
pad_value (depth)
{
    return (pad_info [depth - 1]);
}

/*
 * rast_linelen function... Line length for sun raster files. Different
 * than linelen for xil images.
 */
 
int
rast_linelen (width, depth)
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
 * rast_imagelen function... Most of the work done in linelen function above.
 */
 
int 
rast_imagelen (width, height, depth)
int width;
int height;
int depth;
{
    return ( rast_linelen (width, depth) * height);
}

int 
sn_fwrite (ptr, size, nitems)
char *ptr;
int nitems, size;
{
    return (fwrite (ptr, size, nitems, save_file));
}

static int
get_image(rh, data, pixel_stride, scanline_stride)       /* Get rasterfile image from stream. */
register struct rasterfile *rh;
unsigned char              *data;
int                         pixel_stride;
int			    scanline_stride;
{
    int lineb = rast_linelen (rh->ras_width, rh->ras_depth);
    int imageb = rast_imagelen (rh->ras_width, rh->ras_height, rh->ras_depth);
    int i, j, encoded_len;
    unsigned char *data_ptr = data;
    unsigned char *tmp_data_ptr;
    unsigned char *tmp_data;
  

    if (rh == 0) 
       return(0) ;

    switch (rh->ras_type) {
       case RT_OLD          :
       case RT_STANDARD     :
       case RT_FORMAT_RGB   :  
	    tmp_data = (unsigned char *) malloc (lineb * 
						 sizeof (unsigned char *));
	    if (rh->ras_depth == 32 || 
		(rh->ras_depth == 24 && pixel_stride > 3)) {
	       unsigned char *pp; 
	       for (i = 0; i < rh->ras_height; i++) {
		   im_fread ((char *) tmp_data, 1, lineb);
		   if (rh->ras_depth == 32)
		     pp = tmp_data + 1;
		   else
		     pp == tmp_data;
		   for (j = 0; j < rh->ras_width; j++) {
		       memcpy (data_ptr, pp, 3);		   
		       data_ptr += pixel_stride;
		       if (rh->ras_depth == 32)
  		         pp += 4;
		       else
			 pp += 3;
		       }
		   }
	       }
	    else {
	       for (i = 0; i < rh->ras_height; i++) {
		   im_fread ((char *) tmp_data, 1, lineb);
		   memcpy (data_ptr, tmp_data, scanline_stride);
		   data_ptr += scanline_stride;
		   }
	       }
	    free (tmp_data);
	    break;
 
       case RT_BYTE_ENCODED :  
	    if (lineb == scanline_stride)
	       read_encoded (rh->ras_length, data, imageb);
	    else if (pixel_stride <= 3 || rh->ras_depth != 24) {
	       tmp_data = (unsigned char *) malloc (imageb);
	       read_encoded (rh->ras_length, tmp_data, imageb);
	       tmp_data_ptr = tmp_data;
	       if (rh->ras_depth == 8)
		 encoded_len = lineb;
	       else
		 encoded_len = linelen (rh->ras_width, rh->ras_depth);
	       for (i = 0; i < rh->ras_height; i++) {
		   memcpy (data_ptr, tmp_data_ptr, scanline_stride);
		   data_ptr += scanline_stride;
		   tmp_data_ptr += encoded_len;
		   }
	       free (tmp_data);
	       }
	    else {
	       tmp_data = (unsigned char *) malloc (imageb);
	       read_encoded (rh->ras_length, tmp_data, imageb);
	       tmp_data_ptr = tmp_data;
	       for (i = 0; i < rh->ras_height; i++) {
		 for (j = 0; j < rh->ras_width; j++) { 
		   memcpy (data_ptr, tmp_data_ptr, pixel_stride);
		   data_ptr += pixel_stride; 
		   tmp_data_ptr += 3;
		 }
		 data_ptr += scanline_stride;
		 tmp_data_ptr += lineb;
	       }
	       free (tmp_data);
	       }
       }

    return (1);

}

/* Get image colormap from stream. */

get_image_cmap(rh, image, cmap_type)
register struct rasterfile *rh ;
ImageInfo *image;
int cmap_type;
{
    register int len, error = PIX_ERR ;
 
    len = image->colors;
    if (rh == 0) 
       return(error) ;
 
/* Read colormap data, if any */

    if (len == 0 || cmap_type == RMT_NONE ||
        im_fread((char *) image->red,   1, len) == len &&
        (cmap_type != RMT_EQUAL_RGB ||
        im_fread((char *) image->green, 1, len) == len &&
        im_fread((char *) image->blue,  1, len) == len)) 
       error = 0 ;

    return(error) ;
}


get_image_header(rh)      /* Get Sun rasterfile header from stream. */
register struct rasterfile *rh ;
{

    if (rh == 0) 
       return(PIX_ERR);

    if (im_fread((char *) rh, 1, sizeof(*rh)) == sizeof(*rh))
    {
	unsigned long swaptest = 1;

/* Check to see if the raster file is in the right format i.e for SPARC it   
   should be in SPARC format and for x86 it should be in x86 format */ 

        if (rh->ras_magic == RAS_MAGIC)
            return(0);

        /*if (*(char *) &swaptest)*/

/* If the format is not correct do byte swapping to correct the format for the
   particular architecture */

            swaplong((char *) rh, sizeof(struct rasterfile));

/* Check the format again */

	if (rh->ras_magic == RAS_MAGIC)
	    return(0);
    }
    return(PIX_ERR);

}

int
rast_load(image)
ImageInfo	*image;
{
    int                cmaptype;
    struct rasterfile  rh;
    unsigned char     *image_data, *temp;
    unsigned int       pixel_stride, scanline_stride;

    if (openfile (image->file, image->realfile, image->compression, 
		  image->data, image->file_size) != 0)
       return (-1);

    init_pad ();

    if (get_image_header (&rh) == PIX_ERR) {
       closefile () ;
       return (-1);
       }

    image->width   = rh.ras_width ;
    image->height  = rh.ras_height ;
    image->depth   = rh.ras_depth ;
  

    cmaptype       = rh.ras_maptype ;
    /* If the image is single band byte and has no colormap,
     * then we associate a gray scale colormap with it
     */
    if (cmaptype == RMT_NONE && image->depth == 8) {
	int i;

	image->colors = 256;
	image->red = (unsigned char *)malloc(sizeof(unsigned char) *
	    image->colors);
	image->green = (unsigned char *)malloc(sizeof(unsigned char) *
	    image->colors);
	image->blue = (unsigned char *)malloc(sizeof(unsigned char) *
	    image->colors);

	for (i = 0; i < 256; i++) {
	    image->red[i] = image->green[i] = image->blue[i] = i;
	}
    } else {
	image->colors  = rh.ras_maplength / 3 ;
	image->red     = (unsigned char *) malloc(sizeof (unsigned char) * 
						  image->colors) ;
	image->green   = (unsigned char *) malloc(sizeof (unsigned char) *
						  image->colors) ;
	image->blue    = (unsigned char *) malloc(sizeof (unsigned char) *
						  image->colors) ;
    }

    if (get_image_cmap (&rh, image, cmaptype) == PIX_ERR) {
       closefile () ;
       return (-1);
       }

/*
 * Create the image and get the pointer to the data.
 */

    image_data = (unsigned char *) create_data (image, &pixel_stride,
						&scanline_stride);
    image->bytes_per_line = scanline_stride;

/*
 * Read the file directly into the data pointer.
 */

    if ((get_image (&rh, image_data, pixel_stride, scanline_stride)) == NULL) {
       closefile () ;
       return (-1);
       }

    if (image->depth == 32) 
       image->depth = 24;

    if ((rh.ras_depth >= 24) && (rh.ras_type != RT_FORMAT_RGB)) {
	/* this image is in BGR order */
	image->rgborder = 0;
    }

    closefile ();

    xil_import(image->orig_image, TRUE);

    return(0);

}

static int
read_encoded (incount, out, outcount)
register int 	 incount ;
register u_char *out ;
register int 	 outcount ;
{
    register u_char 	c ;
    register int 	repeat ;

    while (1) {
       while (--incount >= 0 && --outcount >= 0 &&
              (c = im_fgetc()) != ESCAPE) 
	  *out++ = c ;

       if (outcount < 0 || --incount < 0) 
	  break ;

       if ((repeat = im_fgetc()) == 0) 
	  *out++ = c ;
       else {
          if ((outcount -= repeat) < 0 || --incount < 0) 
	     break ;

          c = im_fgetc();
          do
            *out++ = c ;
          while (--repeat != -1) ;
          } 
       }

    if (outcount < 0) 
       incount-- ;
    else if (incount < -1) 
       outcount-- ;

    if ((incount += 2) > 0) 
       return (incount) ;

    return -(++outcount) ;
}

int
rast_save (image)
ImageInfo	*image;
{
    struct rasterfile rh;
    int 	      linesize;
    int len, bytes, lines;
    unsigned char *data;
    unsigned char *tempdata;
    int i, j;
    int            size;
    int            besize, count, npixel;
    unsigned char *beimage;
    unsigned char *bp;
    unsigned char  c, pc = 0;
    unsigned long  swaptest = 1;
    unsigned int   pixel_stride, scanline_stride;
   
    if ((save_file = fopen (image->file, "w"))  == NULL)
       return (-1);

    /* this routine expect data in bgr order */
    if (image->rgborder && image->depth > 8) {
	unsigned int  pixel_stride, scanline_stride;
	unsigned char *image_data;

	image_data = retrieve_data(image->orig_image,
	    &pixel_stride, &scanline_stride);
	swap_red_blue(image, image_data, pixel_stride,
	    scanline_stride);
	xil_import (image->orig_image, TRUE);
    }

    init_pad ();

    rh.ras_magic     = RAS_MAGIC ;
    rh.ras_width     = image->width ;
    rh.ras_height    = image->height ;
    rh.ras_depth     = image->depth ;
    linesize = rast_linelen (rh.ras_width, rh.ras_depth);
    if (image->compression == RUN_LENGTH) 
      rh.ras_type    = RT_BYTE_ENCODED;
    else {
      rh.ras_type    = RT_STANDARD ;
      rh.ras_length = linesize * image->height;
    }

    rh.ras_maptype   = RMT_NONE ;
    rh.ras_maplength = 0;
    data = retrieve_data (image->orig_image, &pixel_stride, &scanline_stride);

    if ((image->depth == 8) && (image->colors > 0)) {
       rh.ras_maplength = image->colors * 3;
       rh.ras_maptype = RMT_EQUAL_RGB;
       }
/*
 * If encoded, do this next to calculate rh.ras_length.
 */
    if (rh.ras_type == RT_BYTE_ENCODED) {
     
      size = scanline_stride * image->height;
      bp = data;

      beimage = (unsigned char *) malloc (size * 3 / 2);  /* worst case */
      if (beimage == NULL)
         return (-1);

      besize = 0;
      count = 0;
      npixel = 0;

      for (i = 0; i < size; ++i) {
	if (rh.ras_depth == 24 && pixel_stride > 3) {
	  npixel++;
	  if (npixel > 3) {
	    bp += (pixel_stride - npixel) + 1;
            i += (pixel_stride - npixel) + 1;
	    npixel = 1;
	  }
	  c = *bp++;
        }
	else 
          c = *bp++;

        if (count > 0) {
          if (pc != c) {
	    if (count == 1 && pc == 128) {
	      beimage[besize++] = 128;
              beimage[besize++] = 0;
              count = 0;
	    }
            else if (count > 2 || pc == 128) {
	      beimage[besize++] = 128;
	      beimage[besize++] = count - 1;
	      beimage[besize++] = pc;
	      count = 0;
            }
	    else {
	      for (j = 0; j < count; ++j)
		beimage[besize++] = pc;
	      count = 0;
            } /* end if count == 1 */
          } /* end if pc != c */
	} /* end if count > 0 */

	pc = c;
	++count;
	if (count == 256) {
	  beimage[besize++] = 128;
	  beimage[besize++] = count - 1;
	  beimage[besize++] = c;
 	  count = 0; 
        }

      }  /* end for */

      if (count > 0) {
	if (count == 1 && c == 128) {
	  beimage[besize++] = 128;
	  beimage[besize++] = 0;
        }
        if (count > 2 || c == 128) {
	  beimage[besize++] = 128;
	  beimage[besize++] = count = 1;
          beimage[besize++] = c;
        }
	else {
	  for (j = 0; j < count; ++j)
	    beimage[besize++] = c;
        }
      } /* end if count > 0 */

      rh.ras_length = besize;

    } /* if encoded */

/*
 * Now we can write out the header.
 */


/*
 * Before writing the header byte swap it if on intel machine.
 */

    if (*(char *) &swaptest) {
	rh.ras_magic=htonl(rh.ras_magic);
        rh.ras_width=htonl(rh.ras_width);
        rh.ras_height=htonl(rh.ras_height);
        rh.ras_depth=htonl(rh.ras_depth);
        rh.ras_length=htonl(rh.ras_length);
        rh.ras_type=htonl(rh.ras_type);
        rh.ras_maptype=htonl(rh.ras_maptype);
        rh.ras_maplength=htonl(rh.ras_maplength);

    }

    if (sn_fwrite ( (char *) &rh, 1, sizeof (rh)) != sizeof (rh))
       return (-1);

/*
 * Colormaps are the same for encoded or standard.
 */
    if ((image->colors > 0) && (image->depth == 8)) {
       if (sn_fwrite ((char *) image->red, 1, image->colors ) 
			!= image->colors)
	  return (-1);
       if (sn_fwrite ((char *) image->green, 1, image->colors) 
			!= image->colors)
	  return (-1);
       if (sn_fwrite ((char *) image->blue, 1, image->colors) 
			!= image->colors)
	  return (-1);
       }

/*
 * Write out image data.
 */

/*
 * Byte swap again just the type of file otherwise the image will not
 * be written.
 */
    rh.ras_type=htonl(rh.ras_type);
    switch (rh.ras_type) {

      case RT_STANDARD:

      if (pixel_stride > 3 && image->depth == 24) {
	unsigned char *lineptr, *startlineptr, *rastlineptr, *raststartptr;
	raststartptr = (unsigned char *) malloc (linesize);
	rastlineptr = raststartptr;
	lineptr = startlineptr = data;

	for (i = 0; i < image->height; i++) {
	  for (j = 0; j < image->width; j++) {
	    memcpy (rastlineptr, lineptr, 3);
	    lineptr += pixel_stride;
	    rastlineptr += 3;
	  }
	  fwrite (raststartptr, 1, linesize, save_file);
	  startlineptr += scanline_stride;
	  lineptr = startlineptr;
	  rastlineptr = raststartptr;
        }
	free (raststartptr);
	
      }
      else {

	lines = image->height ;
	len   = image->bytes_per_line ;

/*
 * If linesize and len are equal, then we can write out the entire
 * image in one big chunk.
 */
	
	if (linesize == len) {
	  linesize *= lines ;
	  len = linesize;
	  lines = 1 ;
	}

/*
 * linesize is the number to write out for each line.
 * len is the bytes per line of the xil image.
 */

	tempdata = (unsigned char *) malloc ((unsigned) linesize);

/*
 * Now, we can write out on scanline, and not strip off extra byte
 * that was there for 24 bit ximages.
 */

	while (--lines >= 0) {
	  memset (tempdata , 0, linesize);
	  memcpy (tempdata, data, linesize);
	  
	  if (sn_fwrite((char *) tempdata, 1, linesize) != linesize)
            return(-1) ;
	  data += image->bytes_per_line;
	}
	
	free (tempdata); 
      }
      break;

      case RT_BYTE_ENCODED:
  
  
        if (fwrite (beimage, 1, besize, save_file) != besize) {
          free (beimage);
          return (-1);
        }
        free (beimage);
        break;
 
      default:
        break;
    }

    xil_import (image->orig_image, FALSE);
    fclose (save_file);

    return (0);

}

