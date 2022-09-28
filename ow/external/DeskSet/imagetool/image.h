
/*
 * @(#)image.h 1.27 94/11/07
 *
 * Copyright (c) 1992 - Sun Microsystems Inc.
 *
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <X11/Xlib.h>
#include <xil/xil.h>

typedef enum {
	RASTER,
	TIF,
	GIF,
	POSTSCRIPT,
	EPSF,
	PICT,
	XPM,
	ILBM,
	PI1,
	PCX,
	PPM,
	PGM,
	PBM,
	JFIF,
#ifdef PHOTOCD
	PCD16,
	PCD4,
	PCD,
	PCDX4,
#endif
	TGA,
	PJT,
	XIMF,
	MTV,
	QRT,
	IMGW,
	SLD,
	SPC,
	SPU,
	GOULD,
	FS1,
	FITS,
	LISPM,
	HIPS,
	MAC,
	ATK,
	BRUSH,
	CMU,
	MGR,
	G3F,
	IMG,
	SICON,
	PI3,
	XBM,
	YBM,
	XWD,
	NO_TYPE
} FileTypes;

typedef struct {
	FileTypes	 type;
	char		*type_name;
	char		*popup_list_name;
	char		*media_type;
	int		(*load_func)();
	int		(*save_func)();
	char		*convert_filter;
 } TypeInfo;

typedef enum {
	NO_COMPRESS,
	UNIX,
	RUN_LENGTH,
	LZW,
	JPEG
} CompTypes;

typedef struct {
	TypeInfo       *type_info;	  /* Type of image		   */
	char	       *file;		  /* Name of image file		   */
	char	       *realfile;	  /* Real file we're reading	   */
	int		file_size;	  /* Size of the file		   */
	char	       *data;		  /* Data if received via tt/dnd   */
	int		free_data;	  /* True if we should free data   */
	unsigned int	width;		  /* Width of image		   */
	unsigned int	height;		  /* Height of image		   */
	unsigned int	nbands;		  /* Number of bands		   */
	XilDataType	datatype;	  /* Datatype of the image	   */
	unsigned int	depth;		  /* Depth of image		   */
	int		bytes_per_line;   /* Bytes per line of image       */
	int             saving;		  /* True if saving out this image */

	/* If true, then the original image is a single band byte image
	 * with more than 216 entries colormap (after colormap compression.
	 * Needs to restore the old colormap before saving
	 */
	int             ditheredto216;

	/* If true, then the original image is three band byte image
	 * dithered to 8-bit for display or saving.
	 * The image data contains an offset, a rescale with -offset is
	 * needed before saving.
	 */
	int             dithered24to8;

	/* If true, then the original image is stored in old_image field,
	 * and the orig_image field has the luminance data of the original
	 * image.
	 */
	int		colortogray;

	int             rgborder;	  /* True if image stored in RGB   */
	CompTypes	compression;	  /* Type of compression	   */
	unsigned char  *red; 		  /* Red values for colors	   */
	unsigned char  *green;		  /* Green values for colors	   */
	unsigned char  *blue;		  /* Blue values for colors	   */
	unsigned char  *old_red; 	  /* original Red values	   */
	unsigned char  *old_green;	  /* original Green values	   */
	unsigned char  *old_blue;	  /* original Blue values	   */
	int	        colors_compressed;/* colors already compressed     */
	XilCis		cis;		  /* Cis if compression = JPEG	   */
	XilImage        orig_image;	  /* Original XilImage		   */
	XilImage        old_image;	  /* place to hold Original Image  */
	XilImage        dest_image;	  /* Result of current operation   */
	XilImage        revert_image;     /* Dithered/Rescaled XilImage    */
	XilImage	view_image;       /* Currently displayed XilImage  */
	Colormap        cmap;		  /* Colormap of image		   */
	int		colors;		  /* Number of colors in image     */
	int		old_colors;	  /* original Number of colors	   */
  	int		offset;		  /* Offset into colormap
					     (where data starts)	   */
 	int		pages;		  /* Number of pages (for ps docs) */
	void		(*rotate_func)(); /* Func for rotate		   */
	void		(*zoom_func)();   /* Func for zoom 		   */
	void		(*hflip_func)();  /* Func for horizontal flip	   */
	void		(*vflip_func)();  /* Func for vertical flip	   */
	void		(*turnover_func)();  /* Func for turning over      */
	void		(*revert_func)(); /* Func for revert		   */
	XilImage	(*display_func)(); /* Func for revert		   */
} ImageInfo; 

extern ImageInfo       *current_image;
extern TypeInfo		all_types [];
extern int		ntypes;

extern int		rast_load ();
extern int		tiff_load ();
extern int		gif_load ();
extern int		ps_load ();
extern int		pict_load ();
extern int		ppm_load ();
extern int		jfif_load ();
extern int		photocd_load ();

extern int		rast_save ();
extern int		tiff_save ();
extern int		gif_save ();
extern int		ps_save ();
extern int		epsf_save ();
extern int		jfif_save ();
/*
extern int		pict_save ();
extern int		ppm_save ();
*/

/* Function prototypes */
extern ImageInfo       *init_image (char *, char *, int, TypeInfo *, char *);
extern void	      	destroy_image (ImageInfo *);
extern void    	        gen_image (ImageInfo *, int);
extern TypeInfo	       *str_to_ftype (char *); 
extern void		copy_image_data (ImageInfo *, ImageInfo *);

/* Image utilities */
extern int		linelen (int, int);
extern unsigned char   *create_data (ImageInfo *, unsigned int *, unsigned int *);
extern unsigned char   *retrieve_data (XilImage, unsigned int *, unsigned int *);
extern int              retrieve_depth (int, XilDataType);

/* Dithering */
extern XilImage	        create_image_from_ps (ImageInfo *);
extern XilImage         expand1to8 (ImageInfo *, int);
extern XilImage	        expand1to24 (ImageInfo *);
extern XilImage	        dither8to1 (ImageInfo *);
extern XilImage	        dither8to4 (ImageInfo *);
extern XilImage	        dither8to8 (ImageInfo *);
extern XilImage	        expand8to24 (ImageInfo *);
extern XilImage	        dither24to1 (ImageInfo *);
extern XilImage	        dither24to4 (ImageInfo *);
extern XilImage	        dither24to8 (ImageInfo *);
extern XilImage	        crop_region (ImageInfo *, XilImage);
extern void             compress_colors (ImageInfo *);
extern void             set_cmap_from_lut (ImageInfo *, XilLookup);

#endif
