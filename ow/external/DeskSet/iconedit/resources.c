#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include "resources.h"

#define  MAXSTRING 80
#define  TRUE       1
#define  FALSE      0

#define  STRCPY       (void) strcpy
#define  SPRINTF      (void) sprintf
#define  FREE         (void) free
#define  GETHOSTNAME  (void) gethostname

extern Display *dpy;

char *iconedit_res[] = {         /* Iconedit resource strings. */
  (char *) NULL, (char *) NULL, (char *) NULL
};

XVars X;

Vars v;

void
load_deskset_defs()     /* Load current deskset resource database. */
{
  char *ptr ;
  char name[MAXPATHLEN] ;
 
  if (X->desksetDB != NULL) XrmDestroyDatabase(X->desksetDB) ;
  if ((ptr = (char*)getenv("DESKSETDEFAULTS")) == NULL)
    {
      SPRINTF(name, "%s/.desksetdefaults", X->home) ;
      X->desksetDB = XrmGetFileDatabase(name) ;
    }
  else X->desksetDB = XrmGetFileDatabase(ptr) ;
}

char *
get_resource(rtype)      /* Get iconedit resource from merged database. */
enum res_type rtype ;
{
  char cstr[MAXSTRING], nstr[MAXSTRING], str[MAXSTRING] ;
  char *str_type[20] ;
  XrmValue value ;

  STRCPY(str, iconedit_res[(int) rtype]) ;

  if (isupper(v->appname[0])) v->appname[0] = tolower(v->appname[0]) ;
  SPRINTF(nstr,  "%s.%s.%s", "deskset", v->appname, str) ;

  if (islower(str[0]))        str[0]        = toupper(str[0]) ;
  if (islower(v->appname[0])) v->appname[0] = toupper(v->appname[0]) ;
  SPRINTF(cstr, "%s.%s.%s", "Deskset", v->appname, str) ;

  if (XrmGetResource(X->rDB, nstr, cstr, str_type, &value) == NULL)
    return((char *) NULL) ;
  else return(value.addr) ;
}

 
/*  Get the resource databases. These are looked for in the following ways:
 *
 *  Classname file in the app-defaults directory. In this case, Classname
 *  is Deskset.
 *
 *  Classname file in the directory specified by the XUSERFILESEARCHPATH
 *  or XAPPLRESDIR environment variable.
 *
 *  Property set using xrdb, accessible through the XResourceManagerString
 *  macro or, if that is empty, the ~/.Xdefaults file.
 *
 *  XENVIRONMENT environment variable or, if not set, .Xdefaults-hostname
 *  file.
 *
 *  DESKSETDEFAULTS environment variable or, if not set, the
 *  ~/.desksetdefaults file
 */

void
load_resources() 
{
  XrmDatabase db ;
  char name[MAXPATHLEN] ;
  char *ptr ;
  char *classname = "Deskset" ;
  int len ;
 
  XrmInitialize() ;
  SPRINTF(name, "/usr/lib/X11/app-defaults/%s", classname) ;
 
/* Get applications defaults file, if any. */
 
  db = XrmGetFileDatabase(name) ;
  XrmMergeDatabases(db, &X->rDB) ;
 
/* Merge server defaults, created by xrdb. If nor defined, use ~/.Xdefaults. */
 
  if (XResourceManagerString(X->dpy) != NULL)
    db = XrmGetStringDatabase(XResourceManagerString(X->dpy)) ;
  else
    { 
      SPRINTF(name, "%s/.Xdefaults", X->home) ;
      db = XrmGetFileDatabase(name) ;
    }
  XrmMergeDatabases(db, &X->rDB) ;

/*  Open XENVIRONMENT file or, if not defined, the .Xdefaults, and merge
 *  into existing database.
 */

  if ((ptr = (char*)getenv("XENVIRONMENT")) == NULL)
    {
      SPRINTF(name, "%s/.Xdefaults-", X->home) ;
      len = strlen(name) ;
      GETHOSTNAME(name+len, 1024-len) ;
      db = XrmGetFileDatabase(name) ;
    }
  else db = XrmGetFileDatabase(ptr) ;
  XrmMergeDatabases(db, &X->rDB) ;

/*  Finally merge in Deskset defaults via DESKSETDEFAULTS or, if not
 *  defined, the ~/.desksetdefaults file.
 */

  if ((ptr = (char*)getenv("DESKSETDEFAULTS")) == NULL)
    {
      SPRINTF(name, "%s/.desksetdefaults", X->home) ;
      db = XrmGetFileDatabase(name) ;
    }
  else db = XrmGetFileDatabase(ptr) ;
  XrmMergeDatabases(db, &X->rDB) ;
}



void
put_resource(rtype, value)  /* Put iconedit resource into deskset database. */
enum res_type rtype ;
char *value ;
{
  char resource[MAXSTRING] ;

  if (isupper(v->appname[0])) v->appname[0] = tolower(v->appname[0]) ;
  SPRINTF(resource, "%s.%s.%s",
          "deskset", v->appname, iconedit_res[(int) rtype]) ;
  XrmPutStringResource(&X->desksetDB, resource, value) ;
}


void
save_resources()
{
  char *ptr ;
  char name[MAXPATHLEN] ;

  if ((ptr = (char*)getenv("DESKSETDEFAULTS")) == NULL)
    {
      SPRINTF(name, "%s/.desksetdefaults", X->home) ;
      XrmPutFileDatabase(X->desksetDB, name) ;
    }
  else XrmPutFileDatabase(X->desksetDB, ptr) ;
}

void
read_resources()      /* Read all possible resources from server. */
{
  struct meter *mp ;
  int curmax, minmax, maxmax ;
  int boolval, i, intval ;
  char *ptr, str[MAXSTRING] ;

  if (get_int_resource(R_HEIGHT,  &intval)) v->height = intval ;
  if (get_int_resource(R_WIDTH, &intval)) v->width  = intval ;
  if (get_int_resource(R_FILEFORM,  &intval)) v->format = intval ;

}



void
write_resources()
{
  char intval[10] ;          /* For converting integer value. */
  char strval[MAXSTRING] ;   /* For converting the list of monitored meters. */
  char *value ;
  int i ;

  load_deskset_defs() ;

  SPRINTF(intval, "%d", v->height) ;
  put_resource(R_HEIGHT, intval) ;

  SPRINTF(intval, "%d", v->width) ;
  put_resource(R_WIDTH, intval) ;

  SPRINTF(intval, "%d", v->format) ;
  put_resource(R_FILEFORM, intval) ;

  save_resources() ;
}

int
get_int_resource(rtype, intval)   /* Get integer resource from the server. */
enum res_type rtype ;
int *intval ;
{
  char *val ;
 
  if ((val = get_resource(rtype)) == NULL) return(0) ;
  *intval = atoi(val) ;
  return(1) ;
}


int
get_str_resource(rtype, strval)   /* Get a string resource from the server. */
enum res_type rtype ;
char *strval ;
{
  char *val ;

  if ((val = get_resource(rtype)) == NULL) return(0) ;
  STRCPY(strval, val) ;
  return(1) ;
}


void
read_str(str, value)
char **str, *value ;
{
  if (*str != NULL) FREE(*str) ;
  if (value != NULL && strlen(value))
    {
      *str = (char *) malloc((unsigned) (strlen(value) + 1)) ;
      STRCPY(*str, value) ;
    }
  else *str = NULL ;
}
 
