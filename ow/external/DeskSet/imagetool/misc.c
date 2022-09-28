
#ifndef lint
static char sccsid[] = "@(#)misc.c 1.20 94/10/03";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

/*
 * misc.c - Miscellaneous functions...
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "image.h"
#include "display.h"
#include "imagetool.h"

#define  SVR4_LIMIT 2048

FILE	*current_file;
char	*current_data = NULL;
int	 ispopen = FALSE;

void
swap_colormap (image)
    ImageInfo  *image;
{
    int i;
    unsigned char *tmp;

    tmp = image->red;
    image->red = image->old_red;
    image->old_red = tmp;

    tmp = image->green;
    image->green = image->old_green;
    image->old_green = tmp;

    tmp = image->blue;
    image->blue = image->old_blue;
    image->old_blue = tmp;

    i = image->colors;
    image->colors = image->old_colors;
    image->old_colors = i; 
}

void
save_colormap (image)
    ImageInfo  *image;
{
    int i;

    if (image->old_red != NULL) {
	free (image->old_red);
	image->old_red = NULL;
    }
    if (image->old_green != NULL) {
	free (image->old_green);
	image->old_green = NULL;
    }
    if (image->old_blue != NULL) {
	free (image->old_blue);
	image->old_blue = NULL;
    }

    image->old_red = (unsigned char *)malloc(sizeof(unsigned char*) *
	    image->colors);
    image->old_green = (unsigned char *)malloc(sizeof(unsigned char*) *
	    image->colors);
    image->old_blue = (unsigned char *)malloc(sizeof(unsigned char*) *
	    image->colors);
    image->old_colors = image->colors;
    memcpy(image->old_red, image->red, image->colors);
    memcpy(image->old_green, image->green, image->colors);
    memcpy(image->old_blue, image->blue, image->colors);
}

void
restore_colormap (image)
    ImageInfo  *image;
{
    int i;

    if (image->red != NULL) {
	free (image->red);
	image->red = NULL;
    }
    if (image->green != NULL) {
	free (image->green);
	image->green = NULL;
    }
    if (image->blue != NULL) {
	free (image->blue);
	image->blue = NULL;
    }

    image->red = (unsigned char *)malloc(sizeof(unsigned char*) *
	    image->old_colors);
    image->green = (unsigned char *)malloc(sizeof(unsigned char*) *
	    image->old_colors);
    image->blue = (unsigned char *)malloc(sizeof(unsigned char*) *
	    image->old_colors);
    image->colors = image->old_colors;
    memcpy(image->red, image->old_red, image->colors);
    memcpy(image->green, image->old_green, image->colors);
    memcpy(image->blue, image->old_blue, image->colors);
}
 
char *
basename (path)
char	*path;
{
    register char *p = strrchr (path, '/');

    if (p == (char *) NULL)
       p = path;
    else
       p++;

    return (p);
}

char *
strip_filename (path)
char	*path;
{
    long	 offset;
    char	*tmp;
    register	char *p = strrchr (path, '/');

    if (path == (char *) NULL)
       return ( (char *) NULL);

    offset = (long) p - (long) path;
    tmp = malloc (offset + 1);
    strncpy (tmp, path, offset);
    tmp [offset] = '\0';
    return (tmp);
}

char *
make_pathname (dir, file)
char    *dir;
char    *file;
{
    char        *fullpath;
    char         tmpdir [MAXPATHLEN];

    ds_expand_pathname (dir, tmpdir);

    if (file [0] == '\0') {
       fullpath = malloc (strlen (tmpdir) + 1);
       strcpy (fullpath, tmpdir);
       } 
    else if (dir [0] == '\0') {
       fullpath = malloc (strlen (file) + 1);
       strcpy (fullpath, file);
       } 
    else {
       fullpath = malloc (strlen (tmpdir) + strlen (file) + 2);
       sprintf(fullpath, DGET("%s/%s"), tmpdir, file);
       } 

    return (fullpath);
}

int
make_tmpfile (tmpfile, filename)
char	**tmpfile;
char	 *filename;
{
    char	tmp_filename [MAXPATHLEN];

/*
 * First check to see if we're going to make the same name.
 * If so, don't!
 */

    sprintf (tmp_filename, "%s.%s", prog->file_template, filename);

    if ((*tmpfile != (char *) NULL) &&
        (strcmp (tmp_filename, *tmpfile) == 0))
       return (0);

    if (*tmpfile != (char *) NULL) {
       unlink (*tmpfile);
       free (*tmpfile);
       }

    *tmpfile = (char *) malloc (strlen (tmp_filename) + 1);
    strcpy (*tmpfile, tmp_filename);
    return (1);
}

int
write_tmpfile (tmpfile, data, filename, size)
char 	**tmpfile;
char	 *data;
char	 *filename;
int	  size;
{
    FILE *fp;
    int	  bytes_written;

/*
 * If make_tmpfile returns 0, then we already have the file
 * created, no need to write out the data again.
 */

    if (make_tmpfile (tmpfile, filename) == 0)
       return (0);

    fp = fopen (*tmpfile, "w");
    chmod(*tmpfile, 0600);
    if (fp == (FILE *) NULL)
       return (-1);

    bytes_written = fwrite (data, sizeof (char), size, fp);
    if (bytes_written != size) {
       unlink (*tmpfile);
       free (*tmpfile);
       *tmpfile = (char *) NULL;
       return (-1);
       }

    fclose (fp);
    return (0);
}
 
void
closefile ()
{
    if (ispopen) {
       pclose (current_file) ;
       ispopen = FALSE ;
       }
    else if (current_file != (FILE *) NULL) {
       fclose (current_file) ;
       current_file = (FILE *) NULL;
       }

    current_data = (char *) NULL;
}

int
openfile (filename, realfilename, compression, data, size)
char 	  *filename ;
char	  *realfilename;
CompTypes  compression;
char	  *data;
int	   size;
{
    char  	 buf[MAXPATHLEN] ;
    char	*tmp_name = realfilename;
 
    ispopen = FALSE;

/*
 * If have data and its compressed, write it out to compfile.
 */

    if (data != (char *) NULL) {
       if (compression == UNIX) {
	  if (write_tmpfile (&(prog->compfile), data, basename (filename), 
			     size) == -1)
	     return (-1); 
          tmp_name = prog->compfile;
	  }
       else {
          current_data = data;
          return (0);
	  }
       }

    if (compression != UNIX) {
       current_file  = fopen (tmp_name, "r");
       if (current_file == (FILE *) NULL) 
          return (-1) ;
       }
    else {
       sprintf (buf, DGET ("/bin/sh -c \"/usr/bin/zcat %s 2>/dev/null\""),
		   tmp_name);
       current_file = popen (buf, "r");
       if (current_file == (FILE* ) NULL) 
          return (-1) ;
       ispopen = TRUE;
       }

    return (0) ;
}     

char *
file_to_open (filename, realfilename, compression, data, size)
char 	  *filename ;
char 	  *realfilename ;
CompTypes  compression;
char	  *data;
int	   size;
{
    char 	 buf [MAXPATHLEN] ;
    char	*tmp_uncompname;
    char	 write_buf [SVR4_LIMIT];
    FILE	*tmp_out;
    char	*tmp_name2;
    char	*tmp_name = realfilename;
    int		 path_len;
    int		 bytes_to_write;
    int		 file_exists;
 

/*
 * If have data need to output it somehow so can get it through
 * filter. If it's compressed, need to run the data through zcat.
 */

    if (data != (char *) NULL) {

/*
 * If data is compressed, write out data to compfile, then run it
 * through zcat into uncompfile.
 */

       if (compression == UNIX) {
	  if (write_tmpfile (&(prog->compfile), data, basename (filename), 
			     size) == -1)
	     return ((char *) NULL); 
          tmp_name = prog->compfile;
	  }

/*
 * Data isn't compressed, just write it out to file.
 */

       else {
	  if (write_tmpfile (&(prog->uncompfile), data, basename (filename), 
			     size) == -1)
	     return ((char *) NULL); 
          tmp_name = prog->uncompfile;
	  }
       }


/*
 * Have file, but it's compressed. Run it through zcat and send
 * output to uncompfile. We do this rather than a uncompress since
 * we may not be able to uncompress it (ie no write permission in
 * the directory, or maybe user doesn't want his file uncompressed!
 */

    if (compression == UNIX)  {
       tmp_name2 = basename (filename);
       path_len = strlen (tmp_name2);
       tmp_uncompname = (char *) malloc (path_len - 1);
       strncpy (tmp_uncompname, tmp_name2, path_len - 2);
       tmp_uncompname [path_len - 2] = '\0';

/*
 * If make_tmpfile returns 0, we already have the file written out.
 */

       file_exists = make_tmpfile (&(prog->uncompfile), tmp_uncompname);
  
       if (file_exists != 0) {
          sprintf (buf, DGET ("/bin/sh -c \"/usr/bin/zcat %s 2>/dev/null\""),
		   tmp_name);
          current_file = popen (buf, "r");
          if (current_file == (FILE *) NULL)
	     return ((char *) NULL);
          tmp_out = fopen (prog->uncompfile, "w");
          if (tmp_out == (FILE *) NULL) {
	     pclose (current_file);
	     return ((char *) NULL);
	     }

          while ((bytes_to_write = fread (write_buf, 1, SVR4_LIMIT, 
					  current_file)) != 0)
	     fwrite (write_buf, 1, bytes_to_write, tmp_out);

          fclose (tmp_out);
          pclose (current_file);
          }
       free (tmp_uncompname);
       tmp_name = prog->uncompfile;
       }

    return (tmp_name);
}

int
openfile_filter (filename, realfilename, compression, data, filter, size)
char 	  *filename ;
char 	  *realfilename ;
CompTypes  compression;
char	  *data;
char	  *filter;
int	   size;
{
    char 	 buf [MAXPATHLEN] ;
    char	*file_name = file_to_open (filename, realfilename, compression,
					   data, size);

    if (file_name == (char *) NULL)
       return (-1);

    ispopen = FALSE;

/*
 * No matter what, we have to do a popen on file_name.
 */

    sprintf (buf, DGET ("/bin/sh -c \"%s %s 2>/dev/null\""), filter, file_name);
    current_file = popen (buf, "r");
    if (current_file == (FILE* ) NULL) 
       return (-1) ;

    ispopen = TRUE;

    return (0) ;
}    
    
int
im_fread (ptr, size, nitems)
char *ptr ;
int nitems, size ;
{
    int len ;
 
    if (current_data == (char *) NULL) {
       if (ispopen) {
          if ((nitems * size) > SVR4_LIMIT) {
             int nobytes = 0;
	     int bytes_to_read;
             int bytes_read = 0;
	     while (1) {
		bytes_to_read = SVR4_LIMIT;
		if ((nobytes + SVR4_LIMIT) > (nitems * size))
		   bytes_to_read = (nitems * size) - nobytes;
                bytes_read = fread (&ptr[nobytes], size, bytes_to_read,
                                      current_file);
                nobytes += bytes_read;
                if (nobytes == (nitems * size))
                   break;
                } 
             return (nobytes);
             }
          }
       return (fread (ptr, size, nitems, current_file)) ;
       }
    else {
       len = size * nitems;
       memcpy (ptr, current_data, len);
       current_data += len;
       return (nitems);
       }
}

int 
im_fgetc ()
{
    int 	  val;
    unsigned char uc_val;

    if (current_data == (char *) NULL)
       val = fgetc (current_file);
    else {
       uc_val = *current_data;
       val = (int) uc_val;
       current_data++;
       }

    return (val);
}

int
check_float_value (value, num)
char    *value;
double  *num;
{
    char        **endptr;

    endptr = (char **) malloc (sizeof (char *));
    *num = strtod (value, endptr);

    if (*endptr != (value + strlen (value)))
       return (-1);

    return (0);
}

/*
 
        Name:	strbm.c
 
 
    Synopsis:	char * strbmencode(strings)
                char * strings[];
 
 		int strbmdestroy(bm)
		char * bm;

		char * strbmexec(addr, nbytes, bm)
		char * addr, * bm;
		int nbytes;


 Description:	These routines provide a fast way of searching large memory 
		areas for the occurrance of one or more strings.  The routines 
		may be used as follows:

		Call strbmencode to generate a compiled search string.  It's 
		argument is a NULL terminated list of pointers to strings.  It 
		returns an opaque handle to the compiled search strategy.  Note
		that it will return NULL if no strings are to be searched for.

		Char * strbmexec runs the compiled search strategy bm returned 
		from strbmencode against memory starting at addr for the 
		specified number of bytes.  It returns a pointer to the 
		beginning of any strings that match the specified search 
		strategy.  Note that to find all the strings, the routine 
		should be called again with the returned value +1 as the 
		new addr.  The routine returns NULL when no matches are found.

		int strbmdestroy destroys the compiled search strategy 
		returned from strbmencode, reclaiming the memory.

		These routines use a Boyer-Moore search algorithm.  The
		longer the search string(s), the faster BM runs.  Note that 
		these routines supports very fast searches of files, using the 
		mmap function. 

*/

void
fast_close (ptr)
mmap_handle_t *ptr ;
{
    (void) munmap (ptr->addr, ptr->len);
    free (ptr);
}

mmap_handle_t *
fast_read (pathname)
char *pathname ;            /* Full pathname of the file to read in. */
{
    struct stat		 buff;
    mmap_handle_t 	*ptr;
    int 		 fd;

    if ((ptr = (mmap_handle_t *) malloc (sizeof (*ptr))) == NULL) 
       return ((mmap_handle_t *) NULL);

    if ((fd = open (pathname, O_RDONLY)) < 0) {
       free (ptr);
       return ((mmap_handle_t *) NULL);
       }
 
    if (fstat (fd, & buff) < 0) {
       close (fd);
       free (ptr);
       return ((mmap_handle_t *) NULL);
       }
 
    if ((ptr->addr = mmap (NULL, ptr->len = (int) buff.st_size, PROT_READ,
                           MAP_PRIVATE, fd, 0)) == (caddr_t) -1) {
       close (fd);
       free (ptr);
       return ((mmap_handle_t *) NULL);
       }

    close (fd);
    return (ptr);
}

void
prepend_ps (fd, pageno)
int	fd;
int	pageno;
{
    FILE	*ps_file = fdopen (fd, "w");

    fprintf (ps_file, "%%%!\n");
    fprintf (ps_file, "userdict begin\n");
    fprintf (ps_file, "/IT_pagestoprint [%d] def\n", pageno);
    fprintf (ps_file, "/IT_nopages 0 def\n");
    fprintf (ps_file, "/IT_showpage /showpage where pop /showpage get def\n");
    fprintf (ps_file, "/IT_arraycontains {\n");
    fprintf (ps_file, "    exch false exch {\n");
    fprintf (ps_file, "        2 index eq {\n");
    fprintf (ps_file, "            pop true exit\n");
    fprintf (ps_file, "        } if\n");
    fprintf (ps_file, "    } forall\n");
    fprintf (ps_file, "    exch pop\n");
    fprintf (ps_file, "} def\n");
    fprintf (ps_file, "/showpage {\n");
    fprintf (ps_file, "    IT_pagestoprint\n");
    fprintf (ps_file, "    IT_nopages 1 add dup /IT_nopages exch store\n");
    fprintf (ps_file, "    IT_arraycontains {\n");
    fprintf (ps_file, "        IT_showpage\n");
    fprintf (ps_file, "    } {\n");
    fprintf (ps_file, "        erasepage\n");
    fprintf (ps_file, "        initgraphics\n");
    fprintf (ps_file, "    } ifelse\n");
    fprintf (ps_file, "} def\n");
    fprintf (ps_file, "end\n");
    fflush (ps_file);
}

int
fast_write (pathname, mem_ptr, size, page)
char *pathname ;        /* Full pathname of where to write the file out. */
char *mem_ptr ;         /* Pointer in memory to the data to be written. */
int size ;              /* Number of bytes to write. */
int page ;		/* Page number of ps file to print out */
{
    int fd, 
	ret ;
 
    if ((fd = open (pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) 
       return(-1) ;

    if (page != 0)
       prepend_ps (fd, page);

    ret = write (fd, mem_ptr, size) ;
 
    if (ret == -1) {
       int s_errno = errno ;         /* Save errno across close call. */
 
       close (fd) ;
       errno = s_errno ;
       return (-1) ;
       }
 
    if (close (fd) == -1) 
       return(-1) ; 		/* Means we couldn't really write the data */

    if (ret != size) {
       unlink (pathname) ;
       return(-1) ;
       }

    return(0) ;
}

encoded_bm *
strbmencode (strings)
char *strings[];
{
    register encoded_bm * ptr;
    register int i;
    static void encode_bm ();
    ptr = (encoded_bm *) malloc (sizeof (*ptr));
  
    for (i=0; strings[i]; i++) ;

    if (i == 0)
       return(NULL);

    ptr->strings = (unsigned char **) malloc ((unsigned)(i * sizeof (char *)));
    ptr->lengths = (unsigned int *) malloc ((unsigned)(i * sizeof (int)));
    encode_bm ((unsigned char **) strings, i, ptr);
    return (ptr);
}

strbmdestroy(ptr)
encoded_bm *ptr;
{
    register int i;

    for (i=0; i<ptr->nstrings; i++)
        free ((char*) ptr->strings[i]);
    free ((char*) ptr->strings);
    free ((char*) ptr->lengths);
    free ((char*) ptr);
}

char *
strbmexec (addr, nbytes, ptr)
unsigned char *addr;
encoded_bm *ptr;
int nbytes;
{
    unsigned char *exec_bm ();

    return ((char *) exec_bm (addr, nbytes, ptr));
}

static void 
encode_bm (s, nstrings, ptr)
register unsigned char *s[];
encoded_bm *ptr;
{
    register int j;
    register unsigned int *sieve = ptr->sieve;

/*
 *  Find shortest search string 
 */	

    ptr->len = -1;
    ptr->nstrings = nstrings;

    for (j=0; j<nstrings; j++) {
        ptr->lengths[j] = strlen ((char*) s[j]);
        ptr->len = Min (ptr->lengths[j], ptr->len);
        ptr->strings[j] = (unsigned char *) 
			  strcpy (malloc ((unsigned)(strlen ((char*) s[j])+1)), 
			 	  (char*) s[j]);
        }

/*
 *  Set max jump distance 
 */

    for (j=0; j<256; j++)
        sieve[j] = ptr->len;

/*
 *  Loop through the search strings, encoded distance to end of string
 *  for each character... Always favor shorter distances.
 */

    for (j=0; s[j]; j++) {
        register unsigned char *ss = s[j];
        register int i;
        register int l = ptr->lengths[j] - 1;


/* 
 * Encode relative position of char in string....
 * later characters have preference
 */

        for (i=0; i<l; i++)     
	    sieve [ss[i]] = Min (sieve [ss[i]], l - i);
        }

/*  
 * mark as being end of string 
 */

    for (j=0; s[j]; j++)  
        sieve[s[j][ptr->lengths[j] - 1]] = (unsigned) -1;  


}    

static unsigned char *
exec_bm (saddr, n, ptr)
encoded_bm *ptr;
unsigned char *saddr;
{
    int i;

    register int inc  = ptr->len;
    register unsigned char *last = saddr + n;
    register unsigned int *sieve = ptr->sieve;
    register unsigned char *addr = saddr;

    unsigned char *ret_value = NULL;

    if (inc > n)
       return (NULL);

    for (i=0; i<256; i++)
        if (sieve[i] == (unsigned) -1)
           sieve[i] = n + inc;
  
    addr += inc - 1;

    while ((addr < last )) {
       while ((addr=addr + sieve[*addr]) < last) ;
       if (addr < (saddr + n + inc))
	  goto exit;
       addr -= n + inc;

       if (ptr->nstrings == 1) { 		/* only one string.... */
  	  register unsigned char *a=addr, 
	                         *b=ptr->strings[0] + ptr->lengths[0] - 1,
                         	 *e=ptr->strings[0];
	  while (*a-- == *b--)
	     if (b < e) {
		ret_value = ++a;
		goto exit;
	        }	
	     goto cont;
	  }

       else {
	  for (i=1; i < inc; i++)
	      if (sieve[*(addr-i)] == inc)    /* a character in minimum set 
					   	 not in any string */
	         goto cont;

	  /* RATS - check each string for match */
	  
	  for (i=0; i < ptr->nstrings; i++) {
	      register unsigned char *a=addr, 
	                             *b=ptr->strings[i] + ptr->lengths[i] - 1,
                                     *e=ptr->strings[i];

	      if ((saddr + ptr->lengths[i]) > addr)   /* not far enough yet */
		 continue;
	      while (*a-- == *b--)
		 if (b < e) {
		    ret_value = ++a;
		    goto exit;
		    }
	      }
	  }

    cont:
       addr++; 			/* force restart on next character */
       continue;
       }

 exit:
     
    for (i=0; i<256; i++)
        if (sieve[i] == (n + inc))
           sieve[i] = (unsigned) -1;

    return (ret_value);
}

int
color_to_gray(image)
    ImageInfo *image;
{
    int nbands, i, j;

    if (image->old_image) {
	xil_destroy(image->old_image);
    }
    image->old_image = image->orig_image;
    image->orig_image = xil_create(image_display->state,
	xil_get_width(image->old_image),
	xil_get_height(image->old_image),
	1, XIL_BYTE);

    nbands = xil_get_nbands(image->old_image);
    if (nbands == 3) {
	xil_set_colorspace(image->old_image,
	    xil_colorspace_get_by_name(image_display->state, "rgblinear"));
	xil_set_colorspace(image->orig_image,
	    xil_colorspace_get_by_name(image_display->state, "ylinear"));
	xil_color_convert(image->old_image, image->orig_image);
    } else if (nbands == 1) {
	Xil_unsigned8 data[256*3];
	XilImage tmp1, tmp2;
	XilMemoryStorageByte storage;
	XilLookup lut;

	for (i = 0, j = 0; i < image->colors; i++) {
	    data[j++] = image->blue[i];
	    data[j++] = image->green[i];
	    data[j++] = image->red[i];
	}

	tmp1 = xil_create(image_display->state, image->colors, 1, 3, XIL_BYTE);
	xil_export(tmp1);
	xil_get_memory_storage(tmp1, (XilMemoryStorage *)&storage);
	storage.data = data;
	xil_set_memory_storage(tmp1, (XilMemoryStorage *)&storage);
	xil_import(tmp1, 1);

	tmp2 = xil_create(image_display->state, image->colors, 1, 1, XIL_BYTE);

	xil_set_colorspace(tmp1,
	    xil_colorspace_get_by_name(image_display->state, "rgblinear"));
	xil_set_colorspace(tmp2,
	    xil_colorspace_get_by_name(image_display->state, "ylinear"));
	xil_color_convert(tmp1, tmp2);

	xil_export(tmp2);
	xil_get_memory_storage(tmp2, (XilMemoryStorage *)&storage);

	lut = xil_lookup_create(image_display->state, XIL_BYTE, XIL_BYTE, 1,
	    image->colors, image->offset, storage.data);
	xil_import(tmp2, 0);

	xil_lookup(image->old_image, image->orig_image, lut);

	xil_destroy(tmp1);
	xil_destroy(tmp2);
	xil_lookup_destroy(lut);
    } else {
	xil_destroy(image->orig_image);
	image->orig_image = image->old_image;
	image->old_image = NULL;
	return 0;
    }

    image->colortogray = 1;
    return 1;
}
