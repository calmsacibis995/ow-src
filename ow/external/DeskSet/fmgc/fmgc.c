#ifndef lint
static char sccsid[]="@(#)fmgc.c	1.7 06/06/96 Copyright 1987-1993 Sun Microsystem, Inc." ;
#endif

/*  Copyright (c) 1987-1993 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>

#define  PATCHLEVEL  5        /* Current patchlevel for this program. */

#define  CLOSEDIR    (void) closedir
#define  FPRINTF     (void) fprintf
#define  FREE        (void) free
#define  MEMSET      (void) memset
#define  SPRINTF     (void) sprintf
#define  STAT        (void) stat
#define  STRCPY      (void) strcpy
#define  UNLINK      (void) unlink

/* Defined to stop some lint errors. */

#ifndef  LINT_CAST
#ifdef   lint
#define  LINT_CAST(arg)  (arg ? 0 : 0)
#else
#define  LINT_CAST(arg)  (arg)
#endif /*lint*/
#endif /*LINT_CAST*/

/* For all function declarations, if ANSI then use a prototype. */

#if  defined(__STDC__)
#define P(args)  args
#else
#define P(args)  ()
#endif  /*__STDC__*/

#ifndef TRUE
#define  TRUE         1
#endif /*TRUE*/

#ifndef FALSE
#define  FALSE        0
#endif /*FALSE*/

#define  DIRINC       128       /* Increment for increasing dir. array. */
#define  MAXLINE      256       /* Maximum length for character strings. */
#define  RECINC       512       /* Increment for increasing rec. array. */

#define  DOTS(A)      (A[0] == '.' && \
                      (A[1] == 0 || (A[1] == '.' && A[2] == 0)))
#define  EQUAL(a, b)  (*a == *b && !strcmp(a, b))
#define  INC          argc-- ; argv++ ;
#define  ZFREE(s)     { if (s != NULL) { FREE(s) ; s = NULL ; } }

typedef struct                  /* GC record structure. */
{
  char *path ;            /* Pathname for this directory. */
  int dsize ;             /* Size in bytes of this directory file. */
  int is_cache ;          /* Set if ".cache" file found. */
  int is_icons ;          /* Set if ".icons" file found. */
  int is_state ;          /* Set if ".state" file found. */
  int fcnt ;              /* Number of files in directory. */
  int size ;              /* Size in bytes of .state and .icons files. */
  time_t mtime ;          /* Last modified time for this pathname. */
} GC_rec ;


char **dirs ;                   /* List of directory names to exclude. */
char *homedir  = NULL ;         /* Users home directory. */
char *progname = NULL ;         /* Name of this program. */
char *username = NULL ;         /* Name of user to be garbage collected. */

GC_rec **recs ;                 /* GC records; one for each dir. under .fm. */

int alloc_dirno = 0 ;           /* Number of allocated directory pointers. */
int alloc_recno = 0 ;           /* Number of allocated GC record pointers. */
int debug       = FALSE ;       /* Set true gives oodles of debug info. */
int dirlen ;                    /* Length of the ~/.fm dir. string. */
int dirno       = 0 ;           /* Number of directories to exclude. */
int limit       = 1500000 ;     /* Low water mark gc limit. */
int recno       = 0 ;           /* Current number of GC records. */
int total       = 0 ;           /* Total size of ~/.fm hierarchy. */
int verbose     = FALSE ;       /* Set true gives verbose messages. */

XrmDatabase rDB ;               /* ~/.desksetdefaults X resources database. */


static char *get_home_dir              P((char *)) ;
static char *get_param                 P((char **, char *)) ;
static char *get_resource              P((char *)) ;
static char *Malloc                    P((int)) ;
static char *make_mem                  P((char *, int, int, int)) ;

static int compare_dir                 P((char **, char **)) ;
static int compare_rec                 P((GC_rec **, GC_rec **)) ;
static int get_int_resource            P((char *, int *)) ;
static int get_str_resource            P((char *, char *)) ;
static int is_excluded                 P((char *)) ;
       int main                        P((int, char **)) ;

static void add_rec                    P((GC_rec *)) ;
static void exclude                    P((char *)) ;
static void get_options                P((int, char **)) ;
static void read_directory             P((char *, struct stat *)) ;
static void read_dirs                  P((char *)) ;
static void read_str                   P((char **, char *)) ;
static void resize_dir_array           P((int, int)) ;
static void resize_rec_array           P((int, int)) ;

static XrmDatabase load_deskset_defs   P((char *)) ;


int
main(argc, argv)
int argc ;
char **argv ;
{
  GC_rec **gc ;
  char fname[MAXPATHLEN], startdir[MAXPATHLEN] ;
  int cur_size ;
  struct stat sbuf ;

  resize_dir_array(TRUE, DIRINC) ;       /* Create initial dir. array. */
  resize_rec_array(TRUE, RECINC) ;       /* Create initial rec. array. */
  get_options(argc, argv) ;              /* Get command line options. */
  read_str((char **) &homedir,
           get_home_dir(username)) ;     /* Get users home directory. */
  read_dirs(homedir) ;                   /* Get other excluded dirs. */

  qsort(dirs, dirno, sizeof(char **), (int (*)()) compare_dir) ;

  SPRINTF(startdir, "%s/.fm", homedir) ;
  dirlen = strlen(startdir) ;
  STAT(startdir, &sbuf) ;
  read_directory(startdir, &sbuf) ;      /* Recursively read ~/.fm. */

  if (total < limit)
    {
      if (verbose)
        {
          FPRINTF(stdout, "Current size of %s is %d bytes.\n",
                           startdir, total) ;
          FPRINTF(stdout, "No need to garbage collect.\n") ;
        }
      exit(0) ;
    }

  qsort(recs, recno, sizeof(GC_rec **), (int (*)()) compare_rec) ;

  cur_size = total ;
  for (gc = recs; gc < recs + recno; gc++)
    {
      if (is_excluded((*gc)->path) != -1) continue ;
      if ((*gc)->is_cache == TRUE)
        {
          SPRINTF(fname, "%s%s/.cache", startdir, (*gc)->path) ;
          if (debug) FPRINTF(stderr, "Removing %s\n", fname) ;
          UNLINK(fname) ;
        }
      if ((*gc)->is_state == TRUE)
        {
          SPRINTF(fname, "%s%s/.state", startdir, (*gc)->path) ;
          if (debug) FPRINTF(stderr, "Removing %s\n", fname) ;
          UNLINK(fname) ;
        }
      if ((*gc)->is_icons == TRUE)
        {
          SPRINTF(fname, "%s%s/.icons", startdir, (*gc)->path) ;
          if (debug) FPRINTF(stderr, "Removing %s\n", fname) ;
          UNLINK(fname) ;
        }
      if ((*gc)->fcnt == 0)
        {
          SPRINTF(fname, "%s%s", startdir, (*gc)->path) ;
          if (rmdir(fname) == 0)
            {
              if (debug) FPRINTF(stderr, "Deleted directory %s\n", fname) ;
              cur_size -= (*gc)->dsize ;
            }
        }

      cur_size -= (*gc)->size ;
      if (cur_size < limit) break ;
    }

  if (verbose)
    FPRINTF(stdout, "%s reduced in size by %d bytes.\n",
                    startdir, total - cur_size) ;
  exit(0) ;
/*NOTREACHED*/
}


static void
add_rec(gc)
GC_rec *gc ;
{
  if (recno >= alloc_recno) resize_rec_array(FALSE, RECINC) ;
  recs[recno++] = gc ;
}


static int
compare_dir(dir1, dir2)    /* Return -1, 0, 1 if less, equal, or greater. */
char **dir1, **dir2 ;
{
  return(strcoll(*dir1, *dir2)) ;
}


static int
compare_rec(rec1, rec2)    /* Return -1, 0, 1 if less, equal, or greater. */
GC_rec **rec1, **rec2 ;
{
       if ((*rec1)->mtime > (*rec2)->mtime) return(-1) ;
  else if ((*rec1)->mtime < (*rec2)->mtime) return(1) ;
  else                                      return(0) ;
}


static void
exclude(dirname)               /* Add this directory to the exclusion list. */
char *dirname ;
{
  if (dirname == NULL || *dirname == '\0') return ;
  if (dirno >= alloc_dirno) resize_dir_array(FALSE, DIRINC) ;
  read_str((char **) &dirs[dirno++], dirname) ;
}


static char *
get_home_dir(username)         /* Get users home directory. */
char *username ;
{
  char *home ;
  struct passwd *pp ;

  if (username == NULL) pp = getpwuid(geteuid()) ;
  else                  pp = getpwnam(username) ;
  if (pp == NULL)
    {
      if ((home = getenv("HOME")) != NULL) return(home) ;
      FPRINTF(stderr, "Unable to get home directory for user.\n") ;
      exit(1) ;
    }
  return(pp->pw_dir) ;
}


static int
get_int_resource(res, intval)   /* Get integer resource from database. */
char *res ;
int *intval ;
{
  char *val ;

  if ((val = get_resource(res)) == NULL) return(0) ;
  *intval = atoi(val) ;
  return(1) ;
}


static void
get_options(argc, argv)               /* Get command line options. */
int argc ;
char **argv ;
{
  char *next ;                        /* The next command line parameter. */

  read_str((char **) &progname, argv[0]) ;
  INC ;
  while (argc > 0)
    {
      if (argv[0][0] == '-')
        switch(argv[0][1])
          {
            case 'D' : debug = TRUE ;
                       break ;

            case 'l' : INC ;
                       next = get_param(argv, "-l needs gc limit") ;
                       limit = atoi(next) ;
                       break ;

            case 'u' : INC ;
                       next = get_param(argv, "-u needs user name") ;
                       read_str((char **) &username, next) ;
                       break ;
 
            case 'v' : FPRINTF(stderr, "%s version 1.%1d\n",
                                       progname, PATCHLEVEL) ;
                       exit(1) ;

            case 'V' : verbose = TRUE ;
                       break ;

            case '?' :
            default  : FPRINTF(stderr, "Usage: %s [-l limit] ", progname) ;
                       FPRINTF(stderr, " [-u username] [-v] [-?] directory0 ") ;
                       FPRINTF(stderr, " directory1 directory2 ...\n") ;
                       exit(1) ;
          }
      else exclude(*argv) ;   /* New directory name to exclude. */
      INC ;
    }
}


static char *
get_param(argv, errmes)
char **argv, *errmes ;
{            
  if (*argv != NULL && argv[0][0] != '-') return(*argv) ;
  else       
    {        
      FPRINTF(stderr, "%s: %s as next argument.\n", progname, errmes) ;
      exit(1) ;
    }        
/*NOTREACHED*/
}


static char *
get_resource(resource)      /* Get X resource from database. */
char *resource ;
{
  char cstr[MAXLINE], nstr[MAXLINE], str[MAXLINE] ;
  char *str_type[20] ;
  XrmValue value ;

  SPRINTF(nstr, "deskset.filemgr.%s", resource) ;

  if (islower(resource[0])) resource[0] = toupper(resource[0]) ;
  SPRINTF(cstr, "Deskset.Filemgr.%s", resource) ;

  if (XrmGetResource(rDB, nstr, cstr, str_type, &value) == FALSE)
    return((char *) NULL) ;
  else return(value.addr) ;
}


static int
get_str_resource(res, strval)    /* Get a string resource from database. */
char *res, *strval ;
{
  char *val ;

  if ((val = get_resource(res)) == NULL) return(0) ;
  STRCPY(strval, val) ;
  return(1) ;
}


static int
is_excluded(name)
char *name ;
{
  int low  = 0 ;
  int high = dirno ;
  int mid, n, reply ;

  while (low < high-1)
    {
      mid = (low + high) / 2 ;
      reply = strcoll(name, dirs[mid]) ;
           if (reply == 0) return(mid) ;
      else if (reply < 0)  high = mid ;
      else                 low  = mid ;
    }
  if ( (dirs [low] ) && (strcoll(name, dirs[low]) == 0) ) n = low ;
  else                               n = -1 ;
  return(n) ;
}


static XrmDatabase
load_deskset_defs(home)
char *home ;
{
  XrmDatabase rDB ;
  char name[MAXPATHLEN], *ptr ;

  if ((ptr = getenv("DESKSETDEFAULTS")) == NULL)
    {
      SPRINTF(name, "%s/.desksetdefaults", home) ;
      rDB = XrmGetFileDatabase(name) ;
    }
  else rDB = XrmGetFileDatabase(ptr) ;
  return(rDB) ;
}


static char *
Malloc(n)
int n ;
{
  char *val ;

  if ((val = calloc(1, (unsigned) n)) == NULL)
    FPRINTF(stderr, "%s: Out of memory.\n", progname) ;
  return(val) ;
}


static char *
make_mem(ptr, old, new, firsttime)
char *ptr ;
int firsttime, old, new ;
{
  char *ret ;

  if (firsttime) return(Malloc(new)) ;
  ret = realloc(ptr, old+new) ;
  MEMSET(ret+old, 0, new) ;
  return(ret) ;
}


static void
read_directory(curdir, dsbuf)     /* Recursively read ~/.fm hierarchy. */
char *curdir ;
struct stat *dsbuf ;
{
  DIR *dirp ;
  GC_rec *gc = NULL ;
  struct dirent *cur ;
  struct stat sbuf ;         /* For statistics on current file. */
  char fname[MAXPATHLEN], *ptr ;

  if ((dirp = opendir(curdir)) != NULL)
    {
      gc = (GC_rec *) LINT_CAST(Malloc(sizeof(GC_rec))) ;
      if (curdir[dirlen]) ptr = &curdir[dirlen] ;
      else                ptr = "/" ;
      read_str((char **) &gc->path, ptr) ;
      if (debug) FPRINTF(stderr, "Adding GC record for %s\n", ptr) ;
      gc->dsize = dsbuf->st_size ;
      gc->mtime = sbuf.st_mtime ;

      while ((cur = readdir(dirp)) != NULL)
        {
          if (DOTS(cur->d_name)) continue ;   /* Is it the . or .. entry? */
          SPRINTF(fname, "%s/%s", curdir, cur->d_name) ;
          STAT(fname, &sbuf) ;
          if ((sbuf.st_mode & S_IFMT) == S_IFDIR)
            read_directory(fname, &sbuf) ;
          if (EQUAL(cur->d_name, ".state") ||
              EQUAL(cur->d_name, ".icons") ||
              EQUAL(cur->d_name, ".cache"))
            {
                   if (EQUAL(cur->d_name, ".cache")) gc->is_cache = TRUE ;
              else if (EQUAL(cur->d_name, ".state")) gc->is_state = TRUE ;
              else                                   gc->is_icons = TRUE ;
              gc->size += sbuf.st_size ;
            }
          else gc->fcnt++ ;
        }
      CLOSEDIR(dirp) ;
      add_rec(gc) ;
      total += gc->dsize + gc->size ;
    }
}


static void
read_dirs(home)
char *home ;
{
  char res[MAXLINE], str[MAXLINE] ;
  int i, intval ;
  int nogoto = 0 ;           /* Number of user goto entries. */
  int nowin  = 0 ;           /* Number of saved windows entries. */

  rDB = load_deskset_defs(home) ;
  STRCPY(res, "noSavedWindows") ;
  if (get_int_resource(res, &intval)) nowin = intval ;
  for (i = 1; i <= nowin; i++)
    {
      SPRINTF(res, "window%1dPath", i) ;
      if (get_str_resource(res, str)) exclude(str) ;
    }

  STRCPY(res, "noUserGotoEntries") ;
  if (get_int_resource(res, &intval)) nogoto = intval ;
  for (i = 0; i < nogoto; i++)
    {
      SPRINTF(res, "userGotoPathname%1d", i) ;
      if (get_str_resource(res, str)) exclude(str) ;
    }
}


static void
read_str(str, value)
char **str, *value ;
{
  ZFREE(*str) ;
  if (value != NULL && strlen(value))
    {
      *str = (char *) malloc((unsigned) (strlen(value) + 1)) ;
      STRCPY(*str, value) ;
    }
  else *str = NULL ;
}


static void
resize_dir_array(f, n)    /* Resize dynamic directory array. */
int f ;                   /* True if first time. Malloc, else realloc. */
int n ;                   /* Initial size or increment. */
{
  dirs = (char **) LINT_CAST(make_mem((char *) dirs,
                     sizeof(char **) * alloc_dirno, sizeof(char **) * n, f)) ;
  alloc_dirno += n ;
}


static void
resize_rec_array(f, n)    /* Resize dynamic directory array. */
int f ;                   /* True if first time. Malloc, else realloc. */
int n ;                   /* Initial size or increment. */
{
  recs = (GC_rec **) LINT_CAST(make_mem((char *) recs,
                  sizeof(GC_rec **) * alloc_recno, sizeof(GC_rec **) * n, f)) ;
  alloc_recno += n ;
}
