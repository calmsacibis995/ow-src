#ifndef lint
static char sccsid[] = "@(#)ce.c 1.17 93/07/01";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <sys/fcntl.h>
#include <sys/param.h>
#include <unistd.h>
#include <desktop/ce.h>
#include <desktop/ce_err.h>
#include "imagetool.h"

#define  UNKNOWN_FILE_TYPE  LGET ("unknown")

static CE_NAMESPACE  file_ns;
static CE_NAMESPACE  type_ns;
static CE_ATTRIBUTE  fns_type;
static CE_ATTRIBUTE  type_template;

/*
 * Initialize stuff we need from the Classing Engine. 
 * 
 */

int
init_ce()
{
  if (ce_begin (0)) 
    return (FALSE);
  
  file_ns = ce_get_namespace_id (DGET ("Files"));
  type_ns = ce_get_namespace_id (DGET ("Types"));
  
  if (!file_ns || !type_ns) 
    return (FALSE);
  
  fns_type = ce_get_attribute_id (file_ns, DGET("FNS_TYPE"));
  type_template = ce_get_attribute_id (type_ns, DGET("TYPE_FILE_TEMPLATE"));

  return (TRUE);
	
}

/*
 * Determine file type. Note that we can determine file type if
 * file is compressed, however, if we're passed data, then 
 * we cannot, since we can't run data through zcat. I suppose 
 * at some point, we should fix this.
 */

char *
type_file (path , compressed, data)
     char *path;
     int  *compressed;
     char *data;
{
  char      buf[64];
  char      tmpbuf[PIPE_BUF];
  int       bufsize = 0;
  int       fd;
  char     *type;
  CE_ENTRY  entry;
  FILE	   *pfd;
  char	   *newpath;
  int	    path_len;
  char	    pbuf [MAXPATHLEN];
 
/*
 * Read the first n bytes of this file for typing.
 * If we're compressed, and have data, return unknown.
 */
 
  if (*compressed == TRUE) {
     if (data != (char *) NULL)
        return (UNKNOWN_FILE_TYPE );

     path_len = strlen (path);
     newpath = (char *) malloc (path_len - 1);
     strncpy (newpath, path, path_len - 2);
     newpath [path_len-2] = '\0';
     sprintf (pbuf, DGET("/usr/bin/zcat %s"), path);
     pfd = popen (pbuf, "r");
     bufsize = fread (buf, 1, sizeof (buf), pfd);
/*
 * Drain the pipe before closing.
 */
     while (fgets (tmpbuf, sizeof (tmpbuf), pfd));
     pclose (pfd);
     }
  else {
     newpath = path;
     if (data != (char *) NULL)
        memcpy (buf, data, sizeof (buf));
     else {
        if ( ( fd = open( path, O_RDONLY ) ) >= 0 ) {
           bufsize = read( fd, buf, sizeof( buf ) );
           close( fd );
           }
        else 
           return (UNKNOWN_FILE_TYPE );
	}
     }

  if (bufsize < -1)
     bufsize = 0;
  
/* 
 * Get the CE entry for this file 
 */

  entry = ce_get_entry( file_ns, 3, newpath, buf, bufsize );
  
  if ( entry ) {
    type = ce_get_attribute( file_ns, entry, fns_type );
    if ( type == NULL ) 
      type = UNKNOWN_FILE_TYPE;
    }
  else
    type = UNKNOWN_FILE_TYPE;
  
/*
 * If we have data and type is compress, then don't bother calling 
 * again since it wont' work and just return compress.
 */

  if (strcmp (type, DGET("compress")) == 0) {
     *compressed = TRUE;
     if (data == (char *) NULL)
        return (type_file (path, compressed, data));
     }

  return( type );
          
}

/*
 *
 */
void
rn_no_template(dir, name, buf, ext)
char *dir, *name, *buf, *ext ;
{
  int     hascopy = FALSE ;  /* Set if filename has "copy" number. */
  char *ptr, *sptr = NULL, tmp[MAXPATHLEN] ;
  int version = 0 ;
 
  /* See if the filename already has a "copy" number. */
 
  if ((sptr = ptr = (char *) strrchr(name, '.')) != (char *)NULL)
    {
      while (*++ptr)
        {
          if (*ptr < '0' || *ptr > '9')
            {
              hascopy = FALSE ;
              version = 0 ;
              break ;
            }
          else 
	    {
	      hascopy = TRUE;
	      version = (version*10) + *ptr - '0' ;
	    }
        }
    }

/* Increment "copy" number (if any), and continue until a unique name. */

  do
    {
      version++ ;
      if (hascopy == TRUE)
        {
          strcpy(tmp, name) ;
          tmp[sptr-name]= '\0' ;

	  if (dir) sprintf(buf, "%s/%s.%d", dir, tmp, version) ;
	  else     sprintf(buf, "%s.%d", tmp, version) ;
	  if (ext)
	     strcat(buf, ext);
        }
      else 
	{ 
	  if (dir) sprintf(buf, "%s/%s.%d", dir, name, version) ;
	  else     sprintf(buf, "%s.%d", name, version) ;
	  if (ext)
	     strcat(buf, ext);
	}
    }
  while (access(buf, F_OK) == 0) ;
}


void
rn_hidden_case(dir, name, buf)
char *dir, *name, *buf;
{
   char *ptr, *sptr = NULL, tmp[MAXPATHLEN] ;
   int version = 0;
   int hascopy = FALSE;
   
   sptr = ptr = name;

   while (*++ptr)
     {
       if (*ptr < '0' || *ptr > '9')
         {
	   if (*ptr != '.') 
	     {
	       hascopy = FALSE;
               version = 0 ;
	     }
           break ;
         }
       else 
	 {
	   hascopy = TRUE;
	   version = (version*10) + *ptr - '0' ;
	 }
     }
   /* Increment "copy" number (if any), and continue until a unique name.  */
   do
     {
       version++ ;
       if (!hascopy)
	  ptr = sptr;
       if (dir) sprintf(buf, "%s/.%d%s", dir, version, ptr) ;
       else     sprintf(buf, ".%d%s", version, ptr) ;
     }
   while (access(buf, F_OK) == 0) ;
}


void
rn_other_cases(dir, name, template, buf)
char *dir, *name, *template, *buf;
{
  char *ptr;
  char tmp[MAXPATHLEN];
  int len;

  if (((ptr = (char *) strchr(name, '.')) == NULL) || 
      (!(*++ptr < '0' || *ptr > '9') && (strchr(ptr, '.') == NULL)))
    {  /* eg. README, README.1, the naming is the same as no template */
       rn_no_template(dir, name, buf, NULL);
       return;
    }


  /*  meeting.ps. will rename to meeting.ps.1. */
  len = strlen(name);
  if (name[len - 1] == '.')
     {
       strcpy(tmp, name);
       tmp[len-1] = '\0';
       rn_no_template(dir, tmp, buf, ".");
       return;
     }

  /* meeting.ps  will rename to meeting.1.ps */
  if ((ptr = (char * ) strrchr(name, (int) '.')) != NULL)
    {
      strcpy(tmp, name);
      tmp[ptr-name] = '\0';
      rn_no_template(dir, tmp, buf, ptr);
      return;
    }
}


/* The rules to name a duplicate file are:

   1. If no file template is found in CE then append ".n" to the filename.
      eg. meeting --> meeting.1, meeting.2, ...

   2. If file template is found in CE and dot(s) isn't the 1st char (ie. not 
      hidden file) then add ".n" infront of the last dot (.) extension.
      eg. meeting.ps    --> meeting.1.ps, meeting.2.ps, ...
          my.meeting.ps --> my.meeting.1.ps, my.meeting.1.ps, ...
          meeting.ps.   --> meeting.ps.1., meeting.ps.2., ...

   3. If file template is found in CE and it starts with dot (ie. hidden file)
      then prepend ".n" to the filename.
      eg. .meeting.ps  --> .1.meeting.ps, .2.meeting.ps, ...
          ..meeting.ps --> .1..meeting.ps, .2..meeting.ps, ...

   4. If file template is found in CE but without dot/extension then
      append ".n" to the filename.
      eg. README --> README.1, README.2, ...
*/
void
increment_copy_number(dir, name, template, buf)
char *dir, *name, *template, *buf ;
{
  if (!template)
     rn_no_template(dir, name, buf, NULL);
  else 
    {
      if (*name == '.') 
	rn_hidden_case(dir, name, buf);/* name starts with dot */
      else
	rn_other_cases(dir, name, template, buf);
    }
}

char *
get_tns_attr(entry, attr)    /* Get the given attribute value from the CE. */
CE_ENTRY entry ;
CE_ATTRIBUTE attr ;
{
  char *str = NULL ;

  if (prog->ce_okay && entry && attr)
    str = (char *) ce_get_attribute(type_ns, entry, attr) ;

  return(str) ;
}

CE_ENTRY
get_tns_entry_by_bits(buf, bufsize)
char *buf ;                          /* Bits to check. */
int bufsize ;                        /* Length of bits. */
{
  CE_ENTRY entry ;
  char *type ;

  if (!prog->ce_okay) return(NULL) ;

  if (bufsize == 0) return(NULL) ;

  entry = (CE_ENTRY) ce_get_entry(file_ns, 3, "", buf, bufsize) ;

  if (!entry)
      return(NULL) ;

  type = ce_get_attribute(file_ns, entry, fns_type) ;
  if (!type)
      return(NULL) ;

  entry = (CE_ENTRY) ce_get_entry(type_ns, 1, type) ;

  return(entry) ;
}

void
unique_filename(name, newname)
char *name, *newname ;
{


       CE_ENTRY  tns_entry;
       char *label = NULL, *template = NULL, *type, *dir = NULL;
       char buf[512], t_dir[MAXPATHLEN];
       int fd, bufsize;
       char *d_ptr = (char *) strrchr(name, '/') ;
       char *e_ptr = d_ptr ;
int file_exists;

  if (access(name, F_OK) == 0)
    file_exists = TRUE;
  else 
    file_exists = FALSE;

       if (prog->ce_okay == -1)
         prog->ce_okay = init_ce();

       if (prog->ce_okay == FALSE) {
         newname = NULL;
         return;
       }

       /*
       while (*e_ptr++) continue ;
       while ((*(e_ptr - 1) != '/') && (e_ptr != d_ptr)) e_ptr-- ;
       */
       if (e_ptr)
          label = ++e_ptr;
       else
	  label = name;

       if (file_exists && (fd=open(name, 0)) !=-1) 
         { 
           bufsize = read(fd, buf, sizeof(buf));
           if( bufsize == -1) bufsize = 0;
           close(fd) ;  
         }
       else {
	 buf[0] = '\0';
         bufsize = 0;
       }

       if (label && *label)
         {
           tns_entry = ce_get_entry(file_ns, 3, label, buf, bufsize) ;
           if (tns_entry)
	     {
	       type = ce_get_attribute(file_ns, tns_entry,
				       fns_type) ;
               if (!type) tns_entry = NULL ;
	       else tns_entry = ce_get_entry(type_ns, 1, type) ;
	     }
         }
       else tns_entry = get_tns_entry_by_bits((char*)buf, bufsize) ;
       template = get_tns_attr(tns_entry, type_template) ;
       if (d_ptr)
         {
          strcpy(t_dir, name);
	  t_dir[d_ptr-name] = '\0';
	  dir = t_dir;
	 }
       increment_copy_number(dir, label, template, (char *) newname) ;
/*
    }
    else
      strcpy (newname, name);
*/

}
