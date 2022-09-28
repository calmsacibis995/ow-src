
#ifndef lint
static char sccsid[] = "@(#)tiff.c 1.27 96/06/18";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

/*
 * Copyright (c) 1988 by Sam Leffler.
 * All rights reserved.
 *
 * This file is provided for unrestricted use provided that this
 * legend is included on all tape media and as a part of the
 * software program in whole or part.  Users may copy, modify or
 * distribute this file at will.
 */

/*
 * Modified from cg8gt.c by D. Berry, Sun Microsystems. 
 * To be integrated into the Snapshot software.
 * December 1989
 */

#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "tiffio.h"

int
tiff_load(image)
ImageInfo *image;
{
    TIFF		*tif;
    short		 bitspersample = 1;
    short		 samplesperpixel = 1;
    register int	 x, y;
    register u_char	*buf, *pp, *src_data;
    int			 lbytes;
    u_short		*r_temp, *g_temp, *b_temp;
    short		 photometric = -1;
    unsigned char  	*image_data, *tmp_data;
    char		*tiff_file = image->realfile;
    unsigned int         scanline_stride, pixel_stride;

/*
 * Check if we have data, or a compressed file first, and
 * do the right thing.
 */

    if ((image->data != (char *) NULL) ||
	(image->compression == UNIX))
       tiff_file = (char *) file_to_open (image->file, image->realfile, 
				 	  image->compression, image->data, 
				 	  image->file_size);


/*
 * OK, now we can start...
 */

    if ((tif = TIFFOpen(tiff_file, "r")) == NULL) 
       return (-1);

    if (TIFFGetField (tif, TIFFTAG_BITSPERSAMPLE, &bitspersample) > 0) {
       if (bitspersample > 8) {
          if (prog->verbose)
             fprintf (stderr, 
		      MGET ("tiff.c: 8-bits per sample max permitted\n"));
          TIFFClose(tif);
          return (-1);
	  }
       }

    if (TIFFGetField (tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel) > 0) {
       if ((samplesperpixel != 1) && (samplesperpixel != 3) && 
	   (samplesperpixel != 4) ) {
          if (prog->verbose)
             fprintf (stderr, 
		    MGET ("tiff.c : 1,3 or 4 channel images only allowed\n")); 
          TIFFClose(tif);
          return (-1);
	  }
       }

    if (TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &(image->width)) == 0) {
/*
       if (prog->verbose)
           printf (stderr, MGET ("tiff.c : No width specified\n"));
*/
       TIFFClose(tif);
       return (-1);
       }

    if (TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &(image->height)) == 0) {
/*
       if (prog->verbose)
           printf (stderr, MGET ("tiff.c : No height specified\n"));
*/
       TIFFClose(tif);
       return (-1);
       }

    TIFFGetField (tif, TIFFTAG_PHOTOMETRIC, &photometric);

    image->depth = bitspersample * samplesperpixel;


    switch (photometric) {
       case PHOTOMETRIC_MINISBLACK :
       case PHOTOMETRIC_MINISWHITE :

		/*
		 * For some strange reason this didn't
		 * work with a 4bpp image. When I forced
		 * it to be 8 it worked.
		 *   !!!!!
		 * NEED MORE IMAGES TO TEST THIS
		 */
	
 	    image->depth = bitspersample;
 	    image_data = (unsigned char *) create_data (image, &pixel_stride,
							&scanline_stride);
	    image->bytes_per_line = scanline_stride;
	    image->colors = (1 << image->depth) ;
	    image->red = (unsigned char *) malloc (sizeof (unsigned char) *
						   image->colors);
	    image->green = (unsigned char *) malloc (sizeof (unsigned char) *
						     image->colors);
	    image->blue = (unsigned char *) malloc (sizeof (unsigned char) *
						    image->colors);
   	    for (x = 0; x < image->colors; x++) {
		if (photometric == PHOTOMETRIC_MINISWHITE) 
		   image->red [x] = (image->colors - x - 1) * 255 
						/ (image->colors - 1);
		else 
		   image->red [x] = (x * 255) / (image->colors - 1);
		image->green [x] = image->red [x];
		image->blue [x] = image->red [x];
		}

	    break;

       case PHOTOMETRIC_PALETTE :
	    image_data = (u_char *) create_data (image, &pixel_stride,
						 &scanline_stride);
	    image->bytes_per_line = scanline_stride;
	    image->colors = (1 << 8);
	    image->red = (unsigned char *) malloc (sizeof (unsigned char) *
						   image->colors);
	    image->green = (unsigned char *) malloc (sizeof (unsigned char) *
						     image->colors);
	    image->blue = (unsigned char *) malloc (sizeof (unsigned char) *
						    image->colors);

		/*
		 * TIFF colour maps should be 16 bits,
		 * so we really should just read them into colours
		 * but for now do it this way.
		 * Should also check for an 8-bit map, maybe for FCS
		 */

	    TIFFGetField(tif, TIFFTAG_COLORMAP, &r_temp, &g_temp, &b_temp);

	    for (x = 0; x < image->colors; x++) {
		image->red [x] = (u_char)(r_temp[x] >> 8);
		image->green [x] = (u_char)(g_temp[x] >> 8);
		image->blue [x] = (u_char)(b_temp[x] >> 8);
		}
	    break;

       case PHOTOMETRIC_RGB :
	    image_data = (unsigned char *) create_data (image, &pixel_stride,
							&scanline_stride);
	    image->bytes_per_line = scanline_stride;
	    break;
       default:
	    if (prog->verbose)
  	      fprintf (stderr, 
		       MGET ("tiff.c: This type of image not supported\n"));
   	    TIFFClose (tif);
	    return (-1);
       }

/*
 * Read in the image data
 */

    buf = (u_char *) malloc (image->width * samplesperpixel * sizeof(u_char));
    if (buf == NULL) {
       if (prog->verbose)
         fprintf (stderr, MGET ("tiff.c: Buf malloc failed\n"));
       TIFFClose (tif);
       return (-1);
       }

    y = 0;
    lbytes = image->bytes_per_line;
    src_data = (u_char *) image_data;
    while ((y < image->height) && TIFFReadScanline(tif, buf, y, 0) > 0) {
       pp = buf;
       if (photometric == PHOTOMETRIC_RGB) {

/*
 * XIL can't handle 32 bit images, so we have to make it a 24.
 */
	   tmp_data = src_data;
	   for (x = 0; x < image->width; x++) {
	       memcpy (src_data, pp, 3);
	       src_data += pixel_stride;
	       pp += samplesperpixel;
	     }
	     src_data = tmp_data + image->bytes_per_line;
	   }
       else if (bitspersample == 4) {
	  for (x = 0; x < image->width / 2;  x++) {
	      src_data[x] = (*pp >> 4) & 0xF;
	      src_data[x+1] = (*pp) & 0xF;
	      pp++;
	      }
	  src_data += lbytes;
	  } 
       else {

/*
 * The data has still to be unpacked
 * This is a cheat as ximage should
 * be stored packed on byte boundaries
 */

	  for (x = 0; x < ((image->width * bitspersample) / 8); x++)
	      src_data[x] = *pp++;

	  src_data += lbytes;
	  }

       y++;
       }

/*
 * If the depth is 32, reset to 24 (since we read in data into
 * only 3 bytes, not 4), and reset bytes per line.
 */

    if (image->depth == 32) {
       image->depth = 24;
       image->bytes_per_line = image->width * 3;
       }

    free ((u_char *)buf);
    TIFFClose (tif);
    xil_import(image->orig_image, TRUE);
    return (0);
}

int
tiff_save (image)
ImageInfo 	*image;
{
    TIFF   	  *save_file;
    unsigned short tmp_red [256];
    unsigned short tmp_green [256];
    unsigned short tmp_blue [256];
    unsigned char *buf, *tmpbuf;
    int		   pixels;
    int		   i, j;
    int		   linewords, lbytes;
    unsigned int   pixel_stride, scanline_stride;
    unsigned char *image_data;
    unsigned char *iptr;

    if ((save_file = TIFFOpen (image->file, "w"))  == NULL)
       return (-1);

    image_data = retrieve_data (image->orig_image, &pixel_stride, 
				&scanline_stride);

    /* this routine expect data in rgb order */
    if (!image->rgborder && image->depth > 8) {
	swap_red_blue(image, image_data, pixel_stride,
	    scanline_stride);
    }

    iptr = image_data;
        
/*
 * Required Fields for TIFF B, G, P, R images
 */

/*
    TIFFSetField(save_file, TIFFTAG_XRESOLUTION, 72.0);
    TIFFSetField(save_file, TIFFTAG_YRESOLUTION, 72.0);
    TIFFSetField(save_file, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
*/
    TIFFSetField(save_file, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
 
    if (image->compression == LZW)
       TIFFSetField(save_file, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    else
       TIFFSetField(save_file, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
 

    switch (image->depth) {
       case 1:	TIFFSetField(save_file, TIFFTAG_PHOTOMETRIC, 
					PHOTOMETRIC_MINISWHITE);
                TIFFSetField(save_file, TIFFTAG_BITSPERSAMPLE, 1);
                TIFFSetField(save_file, TIFFTAG_SAMPLESPERPIXEL, 1);
		pixels = 1;
		lbytes = scanline_stride;
		break;
       case 8:	TIFFSetField(save_file, TIFFTAG_PHOTOMETRIC, 
					PHOTOMETRIC_PALETTE);
                TIFFSetField(save_file, TIFFTAG_BITSPERSAMPLE, 8);
                TIFFSetField(save_file, TIFFTAG_SAMPLESPERPIXEL, 1);
		pixels = 1;
		lbytes = scanline_stride;
	 	for (i = 0; i < image->colors; i++) {
		    tmp_red [i] = (image->red [i] << 8) + 
				   image->red [i];
		    tmp_green [i] = (image->green [i] << 8) + 
				     image->green [i];
		    tmp_blue [i] = (image->blue [i] << 8) + 
				    image->blue [i];
		    }
		for (i = image->colors ; i < 256; i++) {
		    tmp_red [i] = 0;
		    tmp_green [i] = 0;
		    tmp_blue [i] = 0;
		    }
		TIFFSetField(save_file, TIFFTAG_COLORMAP, tmp_red, tmp_green, 
				tmp_blue);
		break;
       case 24: TIFFSetField(save_file, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB); 
		TIFFSetField(save_file, TIFFTAG_BITSPERSAMPLE, 8);
                TIFFSetField(save_file, TIFFTAG_SAMPLESPERPIXEL, 3);
		lbytes = scanline_stride;
		pixels = pixel_stride;
/*
		swap_red_blue (image, image_data, pixel_stride, 
			       scanline_stride);
*/
       }

    TIFFSetField(save_file, TIFFTAG_IMAGEWIDTH, image->width);
    TIFFSetField(save_file, TIFFTAG_IMAGELENGTH, image->height);

    buf = (unsigned char *) malloc (lbytes);
   
#ifdef LATER
    if (image->depth == 24) {
       linewords = image->bytes_per_line / 4;
       ppi = (unsigned int *) image_data;
       } 
    else {
       lbytes = image->bytes_per_line;
       pp = (unsigned char *) image_data;
       }
#endif
    
    for (i = 0; i < image->height ; i++) {

      if (image->depth == 24 && pixel_stride > 3) {
	tmpbuf = buf;
	for (j = 0; j < image->width; j++) {
	  memcpy (tmpbuf, iptr, pixel_stride);
	  tmpbuf += 3;
	  iptr += pixel_stride;
	}
      }
      else {
 	memcpy (buf, iptr, lbytes);
	iptr += scanline_stride;
      }

#ifdef LATER
        switch (image->depth) {
           case 24:  bcntr = 0;
                     for (j = 0; j < image->width; j++) {
                         buf[bcntr++] = ppi[j] & 0xFF;
                         buf[bcntr++] = (ppi[j] >> 8) & 0xFF;
                         buf[bcntr++] = (ppi[j] >> 16) & 0xFF;
                         }
                     ppi += linewords;
                     break;
           case 8:   for (j =0; j < image->width ; j++)
                         buf[j] = pp[j];
                     pp += lbytes;
                     break;
           case 1:   width = image->width / 8 + 1;
                 
                     for (j = 0; j < width; j++) 
                         buf[j] = pp[j];
                     pp += lbytes;
                     break;
           }
#endif

        if (TIFFWriteScanline (save_file, buf, i, 0) < 0) {
	   if (prog->verbose)
             fprintf (stderr, MGET ("%d: Error writing TIFF scanline\n"), 
		      prog->name);
           (void) TIFFClose (save_file);
	   free (buf);
           return (-1);
           }
    }  /* end for */
 
/*
    if (image->depth >= 24) {
       swap_red_blue (image, image_data, pixel_stride, scanline_stride);
       }
*/

    xil_import (image->orig_image, FALSE);
    TIFFClose (save_file);

    free (buf);

    return (0);

}
