#pragma ident	"@(#)OlGetFont.c	302.5	97/03/26 lib/libXol SMI" /* olcommon:src/OlGetFont.c	1.18 */

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *************************************************************************
 *
 * Description:
 *
 *	This file contains the functions _OlGetFont and _OlGetImage
 *	which will return the correct size font or bitmap for a
 *	given screen.
 *
 ******************************file*header********************************
 */


#include <ctype.h>
#include <libintl.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>

#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>


#define MAX_FONT_NAME_SIZE 5		/* this is 8 (DOS max) - 3 (fixed) */
#define OL_BITMAP_PATH	"/usr/X/lib/bitmaps/"
#define OL_MAX_LINE	1000
#define OL_TABSIZE	17	/*  hash table size for reserved words  */
#define OL_FONT		0

struct ref_data {
	union  {
		XFontStruct	*font;
		XImage		*image;
		} ptr;
	int reference_count;
	int multiple_names;
	};

static struct chain {
	char *name;
	Display *display;
	struct ref_data *data;
	struct chain *next;
	} *table[OL_TABSIZE] = {NULL};	/*  hash table  */

/* forward declarations */
static String		construct_filename();
static Boolean		delete_entry(Display *display, XFontStruct *data, int data_type);
static unsigned int	hash_index(char *str);
static XFontStruct *	lookup(char *str, int data_type, Screen *screen, char *install_name);
static int		ReadBitmapFileData(Display *display, Drawable d, char *filename, int *width, int *height, char **data, int *x_hot, int *y_hot);

/*
 *************************************************************************
 *
 *  _OlGetFont - This function will return a XFontStruct for a given
 *	font name, point size, and screen.  If the font cannot be
 *	found, it does a rough best fit.
 *
 ****************************procedure*header*****************************
 */
XFontStruct *
_OlGetFont(Screen *screen, int point_size, String font_name)
{
	String		file_name;
	XFontStruct *	font;
	Cardinal	font_name_size;
	static char	trunc_font_name[MAX_FONT_NAME_SIZE + 1] = { NULL };

	if (screen == NULL) {
		OlWarning(dgettext(OlMsgsDomain,
				"NULL screen passed to _OlGetFont"));
		return(NULL);
	}

	if (font_name == NULL)
		font_name = (String)OL_DEFAULT_FONT_NAME;

	/* resolve name into trunc_name (possibly truncating) */
	font_name_size = _OlMin((int)strlen(font_name), (int)MAX_FONT_NAME_SIZE);
	(void) strncpy(trunc_font_name, (const char *)font_name,
				font_name_size);
	trunc_font_name[font_name_size] = '\0';

	file_name = construct_filename(screen, point_size, trunc_font_name);

	/*
	 *  Look in the cache for this font and return it if it is a hit.
	 */
	font = lookup(file_name, OL_FONT, screen, (char *) NULL);

	if (font == NULL)
		/*
		 *  See if the given name exists in the font directory.
		 */
		font = lookup(trunc_font_name, OL_FONT, screen, file_name);

	XtFree(file_name);
	return(font);

}	/*  _OlGetFont  */


/*
 *************************************************************************
 *
 *  _OlGetImage - This function returns an XImage in the XYBitmap
 *	format for the bitmap data in the named file with the given
 *	point size and screen.  If the given bitmap cannot be found,
 *	then it does a rough best fit.
 *
 ****************************procedure*header*****************************
 */
XImage *
_OlGetImage(Screen *screen, int point_size, char *name)
{
	char *file_name;
	char *path_name;
	XImage *image;


	if (screen == NULL) {
		OlWarning(dgettext(OlMsgsDomain,
				"NULL screen passed to _OlGetImage"));
		return(NULL);
	}

	if (name == NULL) {
		OlWarning(dgettext(OlMsgsDomain,
				"NULL name passed to _OlGetImage\n"));
		return(NULL);
	}

	/*
	 *  Look in the OPEN LOOK bitmap path
	 */
	file_name = construct_filename(screen, point_size, name);
	if ((path_name = (char *) XtMalloc(strlen(file_name) +
	    strlen(OL_BITMAP_PATH) +1)) == NULL) {
		OlWarning(dgettext(OlMsgsDomain,
				"Failed to XtMalloc path name for bitmap\n"));
		return (NULL);
	}
	(void) strcpy(path_name, OL_BITMAP_PATH);
	(void) strcat(path_name, file_name);
	image = (XImage *) lookup(path_name, OL_IMAGE, screen, NULL);

	if (image == NULL) {
		/*
		 *  Now look in the OPEN LOOK bitmap path with just name
		 */
		(void) strcpy(path_name, OL_BITMAP_PATH);
		(void) strcat(path_name, name);
		image = (XImage *) lookup(path_name, OL_IMAGE,
						screen, file_name);
	}

	/*
	 *  Ran out of places to look.
	 */
	XtFree(file_name);
	XtFree(path_name);
	return(image);
}	/*  _OlGetImage  */


/*
 *************************************************************************
 *
 *  OlLoadBitmap - This function reads a given file name and returns
 *	an XImage pointer.
 *
 ****************************procedure*header*****************************
 */
static XImage *
OlLoadBitmap(Screen *screen, char *file_name)
{
	XImage *image;
	Pixmap pixmap;
	Display *display = XDisplayOfScreen(screen);
	char *data;
	int width;
	int height;
	int x_hot_return;
	int y_hot_return;
	int read_status;
	char *buf;

	read_status = ReadBitmapFileData(display,
		RootWindowOfScreen(screen),
		file_name,
		&width,
		&height,
		&data,
		&x_hot_return,
		&y_hot_return);

	switch(read_status) {
	case BitmapOpenFailed:
		if (buf = malloc(256)) {
			(void) snprintf(buf, 256, dgettext(OlMsgsDomain,
			       "_OlGetImage: Could not open file \"%1$s\""),
			       file_name);
			OlWarning(buf);
			free(buf);
		}
		return((XImage *)NULL);
	case BitmapFileInvalid:
		if (buf = malloc(256)) {
			(void) snprintf(buf, 256, dgettext(OlMsgsDomain,
			       "_OlGetImage: Invalid file contents for \"%1$s\""),
			       file_name);
			OlWarning(buf);
			free(buf);
		}
		return((XImage *)NULL);
	case BitmapNoMemory:
		if (buf = malloc(256)) {
			(void) snprintf(buf, 256, dgettext(OlMsgsDomain,
			      "_OlGetImage: Not enough memory for file \"%1$s\""),
			       file_name);
			OlWarning(buf);
			free(buf);
		}
		return((XImage *)NULL);
	default:
		break;
	}

	pixmap = XCreateBitmapFromData(display,
			RootWindowOfScreen(screen),
			data,
			width,
			height);

	image = XGetImage(display,
			pixmap,
			0,
			0,
			width,
			height,
			1,
			XYPixmap);

                        /* Change the format since XPutImage() can only
                         * put images of depth 1 as Bitmap format for
                         * destinations that have depths greater than 1 */

        image->format       = XYBitmap;

	XFreePixmap(display, pixmap);

	return (image);
}	/*  OlLoadBitmap  */

/* Copyright, 1987, Massachusetts Institute of Technology */


static int
ReadBitmapFileData(Display *display, Drawable d, char *filename, int *width, int *height, char **data, int *x_hot, int *y_hot)
	         	        
	        	  
	      		         
	     		      		/* RETURNED */
	     		       		/* RETURNED */
	       		     		/* RETURNED */
	     		      		/* RETURNED */
	     		      		/* RETURNED */
{
	FILE *stream;
	char *ptr;
	char line[OL_MAX_LINE];
	int size, bytes;
	char name_and_type[OL_MAX_LINE];
	char *type;
	int value;
	Boolean version10p = FALSE;
	int padding;
	int bytes_per_line;
	unsigned int ww = 0;
	unsigned int hh = 0;
	int hx = -1;
	int hy = -1;

	*data = 0;

	if (!(stream = fopen(filename, "r")))
		return(BitmapOpenFailed);

	for (;;) {
		if (!fgets(line, OL_MAX_LINE, stream))
			break;
   		if (strlen(line) == OL_MAX_LINE-1) {
			(void) fclose(stream);
			return(BitmapFileInvalid);
			}

		if (sscanf(line, "#define %s %d", name_and_type, &value) == 2) {
			if (!(type = strrchr(name_and_type, '_')))
				type = name_and_type;
			else
				type++;
			if (!strcmp("width", type))
				ww=(unsigned int) value;
			if (!strcmp("height", type))
				hh=(unsigned int) value;
			if (!strcmp("hot", type)) {
				if (type--==name_and_type ||
				    type--==name_and_type)
					continue;
				if (!strcmp("x_hot", type))
					hx = value;
				if (!strcmp("y_hot", type))
					hy = value;
			}
			continue;
		}
    
		if (sscanf(line, "static short %s = {", name_and_type) == 1)
			version10p = TRUE;
		else if (sscanf(line, "static unsigned char %s = {",
					name_and_type) == 1)
			version10p = FALSE;
		else if (sscanf(line, "static char %s = {", name_and_type) == 1)
			version10p = FALSE;
		else continue;

		if (!(type = strrchr(name_and_type, '_')))
			type = name_and_type;
		else
			type++;
		if (strcmp("bits[]", type))
			continue;
    
		if (!ww || !hh) {
			(void) fclose(stream);
			return(BitmapFileInvalid);
			}

		padding = 0;

		bytes_per_line = (ww+7)/8 + padding;
    
		size = bytes_per_line * hh;
		*data = (char *) XtMalloc( size );
		if (!*data) {
			(void) fclose(stream);
			return(BitmapNoMemory);
			}

	    for (bytes = 0, ptr = *data; bytes < size; bytes++, ptr++) {
		if (fscanf(stream, " 0x%x%*[,}]%*[ \n]", &value) != 1) {
			(void) fclose(stream);
			return(BitmapFileInvalid);
		}
		*ptr=(char)value;
	    }

	} /* for */


	if (!*data) {
		(void) fclose(stream);
		return(BitmapFileInvalid);
	}

	*width = ww;
	*height = hh;

	if (x_hot)
		*x_hot = hx;
	if (y_hot)
		*y_hot = hy;

	(void) fclose(stream);
	return(BitmapSuccess);
}


/*
 *************************************************************************
 *
 *  _OlFreeFont - This function free the storage used by the font
 *	structure allocated by a call to _OlGetFont.
 *
 ****************************procedure*header*****************************
 */
void
_OlFreeFont(Display *display, XFontStruct *font)
{

	(void) delete_entry(display, font, OL_FONT);
	
}	/*  _OlFreeFont  */

/*
 *************************************************************************
 *
 *  _OlFreeImage - This function free the storage used by the bitmap image
 *	structure allocated by a call to _OlGetImage.
 *
 ****************************procedure*header*****************************
 */
void
_OlFreeImage(XImage *image)
{

	(void) delete_entry((Display *) NULL, (XFontStruct *) image,
			    OL_IMAGE);

}	/*  _OlFreeImage */


/*
 *************************************************************************
 *
 *  lookup - This function searchs a hash table for the filename given.
 *	If it is found, it returns the XFontStruct or XImage pointer
 *	that is stored in the table.  If this is the first lookup of
 *	a filename, it is read in and entered in the hash table before
 *	it is returned.  This function will return NULL when the filename
 *	does not exist or cannot be read.
 *
 ****************************procedure*header*****************************
 */
static XFontStruct *
lookup(char *str, int data_type, Screen *screen, char *install_name)
          
              		/*  either OL_FONT or BITMAP  */
               
                   
{
	struct chain *p;
	struct chain *new_p;
	unsigned int k;
	XFontStruct *font;
	XImage *image;

	k = hash_index(str);

	for (p = table[k]; p; p = p->next)  {
		if (p->display == DisplayOfScreen(screen) &&
		    (strcmp(str, p->name) == 0))  {
			p->data->reference_count++;
			if (install_name != NULL)  {
				/*
				 *  Create a new entry that points to
				 *  this data.
				 */
				k = hash_index(install_name);
				if ((new_p = (struct chain *)
					XtMalloc(sizeof(struct chain))) == NULL)
					OlError(dgettext(OlMsgsDomain,
						"XtMalloc out of space in _OlGetFont lookup()\n"));
				if ((new_p->name = (char *) XtMalloc((unsigned)
					strlen(install_name) + 1)) == NULL)
					OlError(dgettext(OlMsgsDomain,
						"XtMalloc out of space in _OlGetFont lookup()\n"));
				(void) strcpy(new_p->name, install_name);
				new_p->display = p->display;
				new_p->data = p->data;
				new_p->data->multiple_names++;
				new_p->next = table[k];
				table[k] = new_p;
				}
			return((XFontStruct *) p->data->ptr.font);
			}
		}
	
	/*  str not in table, so make a new entry  */
	switch (data_type)  {
	case OL_FONT:
		if ((font = XLoadQueryFont(XDisplayOfScreen(screen), str)) ==
		    NULL)
			return(NULL);
		break;
	case OL_IMAGE:
		if ((image = OlLoadBitmap(screen, str)) == NULL)
			return(NULL);
		break;
	default:
		OlWarning(dgettext(OlMsgsDomain,
				"Invalid data_type passed to lookup\n"));
		return (NULL);
	}

	if (install_name != NULL)  {
		/*
		 *  Install the data using the default name to avoid
		 *  trying other names the next time it is requested.
		 */
		k = hash_index(install_name);
		}
	
	if ((p = (struct chain *) XtMalloc(sizeof(struct chain))) == NULL)
		OlError(dgettext(OlMsgsDomain,
			"XtMalloc out of space in _OlGetFont lookup()\n"));
	if ((p->data = (struct ref_data *)
	    XtMalloc(sizeof(struct ref_data))) == NULL)
		OlError(dgettext(OlMsgsDomain,
			"XtMalloc out of space in _OlGetFont lookup()\n"));

	if (install_name != NULL)  {
		if ((p->name = (char *)
			XtMalloc((unsigned) strlen(install_name) + 1)) == NULL)
			OlError(dgettext(OlMsgsDomain,
			   "XtMalloc out of space in _OlGetFont lookup()\n"));
		(void) strcpy(p->name, install_name);
		}
	else  {
		if ((p->name = (char *)
			XtMalloc((unsigned) strlen(str) + 1)) == NULL)
			OlError(dgettext(OlMsgsDomain,
			   "XtMalloc out of space in _OlGetFont lookup()\n"));
		(void) strcpy(p->name, str);
		}

	p->display = DisplayOfScreen(screen);
	p->data->reference_count = 1;
	p->data->multiple_names = 1;
	switch (data_type)  {
		case OL_FONT:
			p->data->ptr.font = font;
			break;
		case OL_IMAGE:
			p->data->ptr.image = image;
			break;
		}
	p->next = table[k];
	table[k] = p;
	return((XFontStruct *) p->data->ptr.font);
}


/*
 *************************************************************************
 *
 *  hash_index - this function returns the bucket for the given
 *	string.
 *
 ****************************procedure*header*****************************
 */
static unsigned int
hash_index(char *str)
{
	unsigned int i, k;

	for (k = i = 0; i < 5; i++)
		if (str[i])
			k += str[i];
		else
			break;
	k %= OL_TABSIZE;

	return(k);
}	/*  hash_index  */


/*
 *************************************************************************
 *
 *  delete_entry - This function deletes a given hash table entry if
 *	its reference count is 0.  Otherwise is decrements the
 *	reference count.
 *
 ****************************procedure*header*****************************
 */
static Boolean
delete_entry(Display *display, XFontStruct *data, int data_type)
                 
                  
              		/*  either OL_FONT or OL_IMAGE  */
{
	struct chain *p;
	struct chain *prev = NULL;
	unsigned int i;


for (i = 0; i < OL_TABSIZE; i++)  {
	for (p = table[i]; p; p = p->next)  {
		if (p->data->ptr.font == data)  {
			if (p->data->reference_count > 1)  {
				/*
				 *  Still in use. Decrement count.
				 */
				p->data->reference_count--;
				return(TRUE);
				}
			else  {
				/*
				 *  Delete entry and free space
				 */
				if (prev != NULL)
					prev->next = p->next;
				else
					table[i] = p->next;

				XtFree(p->name);

				switch (data_type)  {
				case OL_FONT:
					XFreeFont(display,
						p->data->ptr.font);
					break;
				case OL_IMAGE:
					XDestroyImage(p->data->ptr.image);
					break;
				default:
					OlError(dgettext(OlMsgsDomain,
						"Invalid data_type passed to delete entry\n"));
					break;
				}
				if (p->data->multiple_names > 1)  {
					/*
					 *  Search the table for entries
					 *  which reference this data, and
					 *  delete them too.
					 */
					int i;
					struct chain *pp;
					struct chain *prev;

					for (i = 0; i < OL_TABSIZE; i++)  {
						prev = NULL;
						for (pp = table[i]; pp; pp = pp->next)  {
							if (pp->data == p->data) {
								if (prev == NULL)
									table[i] = pp->next;
								else
									prev->next = pp->next;
								XtFree(pp->name);
								XtFree((char *)pp);
								if (--(p->data->multiple_names) == 1)
									break;
								}
							prev = pp;
							}
						if (p->data->multiple_names == 1)
							break;
						}
					}
				XtFree((char *)p->data);
				XtFree((char *)p);
				}
			return(TRUE);
			}
		prev = p;
		}
	prev = NULL;
	}
	return(FALSE);
}



/*
 *************************************************************************
 *
 *  construct_filename - this function creates a filename using the
 *	OPEN LOOK font filename format.  This format has the first
 *	character of the name reserved for a display resolution
 *	index.  The second and third characters are reserved for
 *	the point size.  The remainder of the file name is a unique
 *	font or bitmap name.  The caller is responsible for freeing the
 *	string that is returned.
 *
 *	using a default point size is okay here.
 *	'name' cannot be defaulted for bitmaps so must be done by caller.
 *
 *	'screen' & 'name' must be non-NULL
 *
 ****************************procedure*header*****************************
 */
static char *
construct_filename(Screen *screen, int point_size, char *name)
{
	char resolution;
	char *file_name;

	/*
	 *  Get the resolution index of the screen and put it in
	 *	the first character of the filename.
	 */
	resolution = OlGetResolution(screen);

	if ((file_name = (char *) XtMalloc(_OlStrlen(name) + 5)) == NULL) {
		OlWarning(dgettext(OlMsgsDomain,
			"Malloc failed to allocate space for filename"));
		return(NULL);
	}

	if (point_size == 0)
		point_size = OL_DEFAULT_POINT_SIZE;

	(void) sprintf(file_name, "%c%d%s", resolution, point_size, name);

	return(file_name);
}	/*  construct_filename  */


/*
 *************************************************************************
 *
 *  DisplayFontCache - this function is used for debugging the font
 *	cache routines.  It prints out the contents of the chache.
 *	This functin should be used to see if the hash funciton is giving
 *	good distribution.
 *
 ****************************procedure*header*****************************
 */
void
_OlDisplayFontCache(void)
{
	int i;
	struct chain *p;

	for (i = 0; i < OL_TABSIZE; i++)  {
		(void) printf("table[%d]:\n", i);
		for (p = table[i]; p; p = p->next)  {
			(void) printf("\t%s\t%x\t%d\n",
				p->name,
				p->data->ptr.font,
				p->data->reference_count);
			}
		}
	(void) printf("\n");
}	/*  DisplayFontCache  */
