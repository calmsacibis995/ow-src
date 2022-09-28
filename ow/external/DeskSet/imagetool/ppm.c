/*
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
** Copyright (C) 1990 by Mark W. Snitily
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** This tool was developed for Schlumberger Technologies, ATE Division, and
** with their permission is being made available to the public with the above
** copyright notice and permission notice.
*/

#include <ctype.h>
#include <sys/param.h>
#include "ppm.h"
#include "display.h"
#include "image.h"
#include "imagetool.h"

#define MAXVAL    255

FILE *save_file;
extern unsigned char *tmp_red;
extern unsigned char *tmp_green;
extern unsigned char *tmp_blue;

int
ppm_load (image)
    ImageInfo *image;
{
    int readppm(ImageInfo*);

/*
 * Added this.  Open the file.
 */
    if ((image->type_info->type == PPM) || (image->type_info->type == PGM) ||
	(image->type_info->type == PBM)) {
       if (openfile (image->file, image->realfile, image->compression, 
		     image->data, image->file_size) != 0)
         return (-1);
    }
    else {
       if (openfile_filter (image->file, image->realfile, image->compression, 
			    image->data, image->type_info->convert_filter, 
			    image->file_size) != 0)
	  return (-1);
    }

    /* Read in the ppm file. */
    if (!readppm(image)) {
	return -1;
    }

    closefile ();

    return (0);
}

static char
pbm_getc()
{
    register int ich;
    register char ch;

    ich = im_fgetc (); 
    if ( ich == EOF && prog->verbose )
	fprintf (stderr, MGET ("%s: EOF / read error in pbm_getc.\n"), 
		 prog->name);
    ch = (char) ich;
    
    if ( ch == '#' )
	{
	do
	    {
	    ich = im_fgetc( );
	    if ( ich == EOF && prog->verbose )
		fprintf (stderr, MGET ("%s: EOF / read error in pbm_getc.\n"),
			 prog->name);
	    ch = (char) ich;
	    }
	while ( ch != '\n' && ch != '\r' );
	}

    return ch;
}


static int
pbm_getint( )
{
    register char ch;
    register int i;

    do
	{
	ch = pbm_getc( );
	}
    while ( ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' );

    if ( (ch < '0' || ch > '9') && prog->verbose )
	fprintf (stderr, 
		 MGET ("%s: Junk in file where an integer should be in pbm_getint.\n"),
		 prog->name);

    i = 0;
    do
	{
	i = i * 10 + ch - '0';
	ch = pbm_getc( );
        }
    while ( ch >= '0' && ch <= '9' );

    return i;
}

void
ppm_readppminitrest(colsP, rowsP, maxvalP )
    int* colsP;
    int* rowsP;
    pixval* maxvalP;
{
    int maxval;

    /* Read size. */
    *colsP = pbm_getint( );
    *rowsP = pbm_getint( );

    /* Read maxval. */
    maxval = pbm_getint( );
    if ( maxval > PPM_MAXMAXVAL && prog->verbose )
        fprintf (stderr, 
		 MGET ("%s: Maxval is too large - try reconfiguring with PGM_BIGGRAYS or without PPM_PACKCOLORS\n"), 
		 prog->name);
    *maxvalP = maxval;
}

static void
pgm_readpgminitrest( colsP, rowsP, maxvalP )
    int* colsP;
    int* rowsP;
    gray* maxvalP;
{
    int maxval;

    /* Read size. */
    *colsP = pbm_getint( );
    *rowsP = pbm_getint( );

    /* Read maxval. */
    maxval = pbm_getint( );
    if ( maxval > PGM_MAXMAXVAL && prog->verbose )
	fprintf (stderr, 
		 MGET ("%s: Maxval is too large - try reconfiguring with PGM_BIGGRAYS\n"), 
		 prog->name);
    *maxvalP = maxval;
}

static void
pbm_readpbminitrest( colsP, rowsP )
    int* colsP;
    int* rowsP;
{
    /* Read size. */
    *colsP = pbm_getint( );
    *rowsP = pbm_getint( );
}


static int
pbm_readmagicnumber( )
{
    int ich1, ich2;

    ich1 = im_fgetc( );
/*
    if ( ich1 == EOF )
	fprintf( stderr, MGET ("EOF / read error reading magic number 1\n" ));
*/
    ich2 = im_fgetc( );
/*
    if ( ich2 == EOF )
	fprintf( stderr, MGET ("EOF / read error reading magic number 2\n" ));
*/
    return ich1 * 256 + ich2;
}

pixval ppm_pbmmaxval = 1;

static int
ppm_readppminit( colsP, rowsP, maxvalP, formatP )
    int* colsP;
    int* rowsP;
    int* formatP;
    pixval* maxvalP;
{
    int  f;
    /* Check magic number. */
    *formatP = pbm_readmagicnumber( );
    f = (PPM_FORMAT_TYPE (*formatP));
    switch (f)
	{
	case PPM_TYPE:
	ppm_readppminitrest( colsP, rowsP, maxvalP );
	break;

	case PGM_TYPE:
	pgm_readpgminitrest( colsP, rowsP, maxvalP );
	break;

	case PBM_TYPE:
	pbm_readpbminitrest( colsP, rowsP );
	*maxvalP = ppm_pbmmaxval;
	break;

	default:
	if (prog->verbose)
 	  fprintf (stderr, 
		   MGET ("%s: Bad magic number - not a ppm, pgm, or pbm file.\n"),
		   prog->name);
	return 0;
	}
    return 1;
}


static unsigned char
pbm_getrawbyte( )
{
    register int iby;

    iby = im_fgetc( );
    if ( iby == EOF && prog->verbose )
	fprintf (stderr, MGET ("%s: EOF / read error in pbm_getrawbyte.\n"),
		 prog->name);
    return (unsigned char) iby;
}

static bit
pbm_getbit( )
{
    register char ch;

    do
	{
	ch = pbm_getc( );
	}
    while ( ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' );

    if ( ch != '0' && ch != '1' && prog->verbose )
	fprintf (stderr, 
		 MGET ("%s: Junk in file where bits should be in pbm_getbit.\n"),
		 prog->name);

    return ( ch == '1' ) ? 1 : 0;
}

int
readppm(image)
    ImageInfo *image;
{
    int cols, rows;
    pixval maxval;
    int format;
    XilMemoryStorageByte storage;
    unsigned char *image_data;

    int (*fptr)(unsigned char*, unsigned int, unsigned int);
    int load_ppmdata(unsigned char*, unsigned int, unsigned int);
    int load_rppmdata(unsigned char*, unsigned int, unsigned int);
    int load_pgmdata(unsigned char*, unsigned int, unsigned int);
    int load_rpgmdata(unsigned char*, unsigned int, unsigned int);
    int load_pbmdata(unsigned char*, unsigned int, unsigned int);
    int load_rpbmdata(unsigned char*, unsigned int, unsigned int);

    if (!ppm_readppminit(&cols, &rows, &maxval, &format))
       return(0);

    image->width = cols;
    image->height = rows;
    if (image->width == 0 || image->height == 0) {
	return(0);
    }

    /* create proper type of image accroading to the format */
    switch (format) {
	case PPM_FORMAT:
	    image->depth = 24;
	    image->nbands = 3;
	    image->datatype = XIL_BYTE;
	    fptr = load_ppmdata;
	break;
	case RPPM_FORMAT:
	    image->depth = 24;
	    image->nbands = 3;
	    image->datatype = XIL_BYTE;
	    fptr = load_rppmdata;
	break;
	case PGM_FORMAT:
	    image->depth = 8;
	    image->nbands = 1;
	    image->datatype = XIL_BYTE;
	    fptr = load_pgmdata;
	break;
	case RPGM_FORMAT:
	    image->depth = 8;
	    image->nbands = 1;
	    image->datatype = XIL_BYTE;
	    fptr = load_rpgmdata;
	{
	    /* There is no colormap - I will make one now. DDT */

	    int i;

	    image->colors = 256;
	    image->red = malloc(image->colors);
	    image->green = malloc(image->colors);
	    image->blue = malloc(image->colors);
	    for (i = 0; i < image->colors; i++)
		image->red[i] = image->green[i] = image->blue[i] = i;
	}
	break;
	case PBM_FORMAT:
	    image->depth = 8;
	    image->nbands = 1;
	    image->datatype = XIL_BYTE;
	    fptr = load_pbmdata;
	break;
	case RPBM_FORMAT:
	    image->depth = 1;
	    image->nbands = 1;
	    image->datatype = XIL_BIT;
	    fptr = load_rpbmdata;
	break;
    }

    image->orig_image = xil_create(image_display->state,
	image->width, image->height, image->nbands, image->datatype);
    xil_export(image->orig_image);
    xil_get_memory_storage(image->orig_image, (XilMemoryStorage *)&storage);
    image->bytes_per_line = storage.scanline_stride;
    image_data = (unsigned char *)malloc(image->width * image->height *
	image->nbands);
    if (!image_data) {
	if (prog->verbose) {
	    fprintf(stderr, MGET("%s: Unable to allocate ppm data.\n"),
		prog->name);
	}
	return(0);
    }
    storage.data = image_data;

    if (!(fptr)(image_data, image->width, image->height)) {
	return(0);
    }

    xil_set_memory_storage(image->orig_image, (XilMemoryStorage *)&storage);
    xil_import(image->orig_image, TRUE);
    image->rgborder = 0;

    if ((format == PPM_FORMAT || format == RPPM_FORMAT) && maxval != 255) {
	float c;

	c = 255.0 / (float)maxval;
	xil_multiply_const(image->orig_image, &c, image->orig_image);
    }

    return(1);
}

int
load_ppmdata(image_data, width, height)
    unsigned char *image_data;
    unsigned int width, height;
{
    unsigned int i, j;

/* NOTE:
printf("read ppm file\n");
 */

    for (i = 0; i < height; i++) {
	for (j = 0; j < width; j++) {
	    image_data[2] = pbm_getint();
	    image_data[1] = pbm_getint();
	    image_data[0] = pbm_getint();
	    image_data += 3;
	}
    }
    return(1);
}

int
load_rppmdata(image_data, width, height)
    unsigned char *image_data;
    unsigned int width, height;
{
    unsigned int i, j;

/* NOTE:
printf("read rppm file\n");
*/

    for (i = 0; i < height; i++) {
	for (j = 0; j < width; j++) {
	    image_data[2] = pbm_getrawbyte( );
	    image_data[1] = pbm_getrawbyte( );
	    image_data[0] = pbm_getrawbyte( );
	    image_data += 3;
	}
    }
    return(1);
}

int
load_pgmdata(image_data, width, height)
    unsigned char *image_data;
    unsigned int width, height;
{
    unsigned int i, j;

/* NOTE:
printf("read pgm file\n");
*/

    for (i = 0; i < height; i++) {
	for (j = 0; j < width; j++) {
	    *image_data++ = pbm_getint( );
	}
    }
    return(1);
}

int
load_rpgmdata(image_data, width, height)
    unsigned char *image_data;
    unsigned int width, height;
{
    unsigned int nitems;

/* NOTE:
printf("read rpgm file\n");
*/

    nitems = height * width;

    if (im_fread(image_data, sizeof(char), nitems) == nitems) {
	return(1);
    } else {
	return(0);
    }
}

int
load_pbmdata(image_data, width, height)
    unsigned char *image_data;
    unsigned int width, height;
{
    unsigned int i, j;

/* NOTE:
printf("read pbm file\n");
*/

    for (i = 0; i < height; i++) {
	for (j = 0; j < width; j++) {
	    *image_data++ = pbm_getint() ? 0 : 255;
	}
    }
    return(1);
}

int
load_rpbmdata(image_data, width, height)
    unsigned char *image_data;
    unsigned int width, height;
{
    unsigned int nitems;

/* NOTE:
printf("read rpbm file\n");
*/

    nitems = height * ((width + 7) / 8);

    if (im_fread(image_data, sizeof(char), nitems) == nitems) {
	return(1);
    } else {
	return(0);
    }
}
