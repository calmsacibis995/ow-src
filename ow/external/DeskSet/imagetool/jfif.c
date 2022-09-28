
#ifndef lint
static char sccsid[] = "@(#)jfif.c 1.18 96/06/18";
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

#include "display.h"
#include "image.h"
#include "imagetool.h"

FILE 		*save_file;
unsigned char 	*cis_data;

static XilColorspace  ycc_colorspace = NULL;  
static XilColorspace  rgb_colorspace = NULL;

void
jfif_done_with_data (data)
void 	*data;
{
    free (cis_data);
    cis_data = (unsigned char *) NULL;
}

int
jfif_load(image)
ImageInfo	*image;
{
    XilImageType	 xiltype;
    XilDataType		 datatype;
    unsigned int 	 nbands, width, height;
    XilImage		 tmp_image;
    Xil_boolean		 success;
    XilMemoryStorage	 storage;
    unsigned char	*image_data;
    int			 i;
    unsigned int         pixel_stride, scanline_stride;

    image->compression = JPEG;
    if (openfile(image->file, image->realfile, image->compression, image->data,
		 image->file_size) != 0)
       return (-1);

    image->cis = xil_cis_create (image_display->state, "Jpeg");

    if (cis_data != (unsigned char *) NULL)
       free (cis_data);

    cis_data = (unsigned char *) calloc (1, image->file_size);

    if (im_fread (cis_data, 1, image->file_size) != image->file_size)
       return (-1); 

    closefile ();

    xil_cis_put_bits_ptr (image->cis, image->file_size, 1, cis_data, 
		          jfif_done_with_data);
    
    xiltype = xil_cis_get_output_type (image->cis);
    xil_imagetype_get_info (xiltype, &width, &height, &nbands, &datatype); 
  
    image->width = width;
    image->height = height;
    image->depth = retrieve_depth (nbands, datatype); 
 
    image->compression = JPEG;

    image->orig_image = xil_create (image_display->state, image->width,
                        	    image->height, nbands, datatype);

    image_data = retrieve_data (image->orig_image, &pixel_stride,
				&scanline_stride);
    xil_import (image->orig_image, FALSE);
    image->bytes_per_line = scanline_stride;

    xil_decompress (image->cis, image->orig_image); 

    if (nbands > 1) {
      float scale[3], offset[3];
      int    i;
/*
 * Scale the image down from 0-255 to 16-235.
 */
      scale[0] = (235.0 - 16.0) / 255.0;
      scale[1] = (240.0 - 16.0) / 255.0;
      scale[2] = (240.0 - 16.0) / 255.0;
      
      offset[0] = 16.0;
      offset[1] = 16.0;
      offset[2] = 16.0;

      xil_rescale (image->orig_image, image->orig_image, scale, offset);
      
/*
 * Convert from ycc to rgb colorspace
 */

       if (ycc_colorspace == NULL) 
          ycc_colorspace = xil_colorspace_get_by_name 
                                     (image_display->state, DGET ("ycc601"));
       xil_set_colorspace (image->orig_image, ycc_colorspace);

       tmp_image = xil_create (image_display->state, image->width,
                               image->height, nbands, datatype);

       if (rgb_colorspace == NULL)
          rgb_colorspace = xil_colorspace_get_by_name
                         	(image_display->state, DGET ("rgb709"));
       xil_set_colorspace (tmp_image, rgb_colorspace);
       xil_color_convert (image->orig_image, tmp_image);
       xil_destroy (image->orig_image);
       image->orig_image = tmp_image;
 
/*
       image_data = retrieve_data (image->orig_image, &pixel_stride,
				   &scanline_stride);
       swap_red_blue (image, image_data, pixel_stride, scanline_stride);
       xil_import (image->orig_image, FALSE);
*/
	image->rgborder = 0;
       }
    else {
       image->colors = 256;
       image->red = (unsigned char *) malloc (sizeof (unsigned char) *
                                              image->colors) ;
       image->green = (unsigned char *) malloc (sizeof (unsigned char) *
                                                image->colors) ;
       image->blue = (unsigned char *) malloc (sizeof (unsigned char) *
                                               image->colors) ;
       for (i = 0; i < 256; i++) {
	   image->red [i] = i;
	   image->green [i] = i;
	   image->blue [i] = i;
	   }
       }


    return (0);

} 


static int
write_soi_marker (file, data, nbytes)
    FILE           *file;
    unsigned char  *data;
    int            *nbytes;
{
    unsigned char buf[1];
/*
    if ((*nbytes >= 4) &&
        ((int) data[0] == 0xff) &&
	((int) data[1] == 0xd8) &&
        ((int) data[2] == 0xff) &&
        ((int) data[3] == 0xe0)) {
      fwrite ((char *) data, sizeof (unsigned char), 4, file);
      data += 4;
      *nbytes -= 4;
      return (1);
    }
    else if ((*nbytes >= 3) &&
             ((int) data[0] == 0xff) &&
	     ((int) data[1] == 0xd8) &&
	     ((int) data[2] == 0xff)) {
      fwrite ((char *) data, sizeof (unsigned char), 3, file);
      data += 3;
      *nbytes -= 3;
      buf[0] = 0xe0;
      fwrite ((char *) buf, sizeof (unsigned char), 1, file);
      return (1);
    }
*/
    if ((*nbytes >= 2) &&
	((int) data[0] == 0xff) &&
	((int) data[1] == 0xd8)) {
      fwrite ((char *) data, sizeof (unsigned char), 2, file);
      *nbytes -= 2;
      return (1);
    }
    else
      return -1;
}

static int
write_jfif_header (save_file)
    FILE  *save_file;
{
    unsigned char  header[20];
    int            nbytes;

    header[0] = 0xff;
    header[1] = 0xe0;
    header[2] = 0x00;  /* length[0] */
    header[3] = 0x10;  /* length[1] */
    header[4] = 0x4a;  /* 'J' */
    header[5] = 0x46;  /* 'F' */
    header[6] = 0x49;  /* 'I' */
    header[7] = 0x46;  /* 'F' */
    header[8] = 0x00;  /* NUL */
    header[9] = 0x01;  /* version[0] */
    header[10] = 0x01;  /* version[1] */
    header[11] = 0x00;  /* No Units */
    header[12] = 0x00; /* xdensity[0] */
    header[13] = 0x01; /* xdensity[1] */
    header[14] = 0x00; /* ydensity[0] */
    header[15] = 0x01; /* ydensity[1] */
    header[16] = 0x00; /* x thumbnail */
    header[17] = 0x00; /* y thumbnail */
    if ((nbytes = fwrite ((char *) header, sizeof (unsigned char), 18,
	      save_file)) == 18)
      return 1;
    else
      return 0;

    
}

int
jfif_save (image)
ImageInfo	*image;
{
    char		*colorspace;
    unsigned int	 width, height, nbands;
    XilDataType		 datatype;
    int			 nbytes, frames;
    unsigned char	*image_data;
    XilImage		 new_image;
    int                  wrote_soi = FALSE;
    int                  wrote_jfif = FALSE;

    if ((save_file = fopen (image->file, "w"))  == NULL)
       return (-1);

    image->cis = xil_cis_create (image_display->state, "Jpeg");

    xil_get_info (image->orig_image, &width, &height, &nbands, &datatype);

    new_image = xil_create (image_display->state, width, height, nbands,
			    datatype);
    
    if (rgb_colorspace == NULL)
      rgb_colorspace = xil_colorspace_get_by_name
      (image_display->state, DGET ("rgb709"));
    xil_set_colorspace (image->orig_image, rgb_colorspace);

    if (ycc_colorspace == NULL) 
      ycc_colorspace = xil_colorspace_get_by_name 
      (image_display->state, DGET ("ycc601"));
    xil_set_colorspace (new_image, ycc_colorspace);
    
    xil_color_convert (image->orig_image, new_image);

/*
 * Scale to props YCC601 ranges:
 * Y = 16-235, C = 16-240, C = 16-240.
 */
    if (nbands == 3) {
      float  scale[3], offset[3];
      int     i;
      
      scale[0] = 255.0 / (235.0 - 16.0);
      scale[1] = 255.0 / (240.0 - 16.0);
      scale[2] = 255.0 / (240.0 - 16.0);

      offset[0] = -16.0 * scale[0];
      offset[1] = -16.0 * scale[1];
      offset[2] = -16.0 * scale[2];

      xil_rescale (new_image, new_image, scale, offset);
    }
 
    xil_compress (new_image, image->cis);

/*
 * This writes out...
 * 1. SOI Marker
 * 2. JFIF Header
 * 3. JPEG Data
 *
 * Note that if the number of bytes read initially is not
 * at least 3, the SOI Marker will not be written out and it 
 * will fail.  Need to find a way to "append" a second read
 * if not enough bytes read the first time.
 */

    while (xil_cis_has_data (image->cis) > 0) {
        
      image_data = (unsigned char *) xil_cis_get_bits_ptr (image->cis,
							   &nbytes, &frames);
      if (wrote_soi == FALSE && nbytes > 3) {
        if (write_soi_marker (save_file, image_data, &nbytes) > 0)
	  image_data += 2;
 	  wrote_soi = TRUE;
      }
      
      if (wrote_soi == TRUE && wrote_jfif == FALSE) {
	if (write_jfif_header (save_file) > 0)
  	  wrote_jfif = TRUE;
      }

      if (wrote_soi && wrote_jfif) 
         fwrite ( (char *) image_data, sizeof (unsigned char), nbytes,
			   save_file);
      else {
	fclose (save_file);
	if (new_image != image->orig_image)
	  xil_destroy (new_image);
	return (-1);
      }
    }

    if (new_image != image->orig_image)
       xil_destroy (new_image);
    fclose (save_file);
    return (0);

}
