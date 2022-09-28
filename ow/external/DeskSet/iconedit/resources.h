#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#define DGET(s)  (s)

enum res_type { R_HEIGHT,  R_WIDTH, R_FILEFORM };

enum var_type     { V_USAGE,  V_TRUE,    V_FALSE,   V_RESN,      /* Various. */
                    V_RESC,   V_LHOST,   V_IADDR,   V_SIGNAL,
                    V_FORK,   V_CLNTERR, V_PROPS,   V_FNAME,
                    V_FLABEL, V_ILABEL,  V_MTITLE,  V_CMDNAME } ;

typedef struct Xobject {
  XrmDatabase desksetDB ;     /* Deskset resources database. */
  XrmDatabase rDB ;           /* Combined resources database. */
  char *home ;                /* Pointer to user's home directory. */
  Display *dpy ;              /* Display ids of iconedit's frames. */
} XObject;

typedef struct Xobject *XVars ;


struct iconVars {
  char *appname;              /* Application name to use for resources. */
  int   height;               /* Height of icon when no file is read in... */
  int   width;                /* Width of icon when no file is read in... */
  int   format;               /* Default save format 1 icon, 2 xbm, */
} IconVars;                   /* 3 color xpm, 4 mono xpm */

typedef struct iconVars *Vars ;






