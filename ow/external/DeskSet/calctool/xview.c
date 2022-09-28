#ifndef lint
static char sccsid[] = "@(#)xview.c 1.35 96/06/05 Copyr 1987 Sun Micro";
#endif

/*  Copyright (c) 1987 - 1990, Sun Microsystems, Inc.  All Rights Reserved.
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
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <euc.h>
#include <fcntl.h>
#include "color.h"
#include "calctool.h"
#include "ds_popup.h"
#include "extern.h"
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/notice.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/svrimage.h>
#include <xview/xv_xrect.h>
#include <xview/font.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>
#include "ds_xlib.h"

#include "calctool_ui.h"

#define  WINDOW_CLASS_RES     "OpenWindows.WindowColor"
#define  WINDOW_RES           "openwindows.windowcolor"
#define  WORKSPACE_CLASS_RES  "OpenWindows.WorkspaceColor"
#define  WORKSPACE_RES        "openwindows.workspacecolor"

#define  DEFFONT              "fixed"

#define  DS_ADD_HELP                  (void) ds_add_help
#define  DS_CENTER_ITEMS              (void) ds_center_items
#define  DS_GET_FRAME_SIZE            (void) ds_get_frame_size
#define  DS_SAVE_CMDLINE              (void) ds_save_cmdline
#define  DS_SET_FRAME_SIZE            (void) ds_set_frame_size
#define  NOTIFY_INTERPOSE_EVENT_FUNC  (void) notify_interpose_event_func
#define  SELN_QUERY                   (void) seln_query
#define  XV_DESTROY_SAFE              (void) xv_destroy_safe
#define  XV_GET                       (void) xv_get
#define  XV_SET                       (void) xv_set

typedef struct Xobject {            /* XView/Xlib graphics object. */
  Canvas_paint_window pw[MAXFCP] ;
  Event *cur_event ;
  Server_image corner_sv[MAXIMAGES] ;
  Server_image svbut_pr[MAXIMAGES] ;
  Menu menus[MAXMENUS] ;
  Seln_client sel_client ;
  Seln_holder holder ;
  Seln_rank rank ;
#ifdef OW_I18N
  XFontSet fontset ;
  Xv_Font pf ;
#endif /*OW_I18N*/

  enum menu_type CFtype ;

  Display *dpy ;                    /* Display ids of calctool frames. */
  Drawable xid[MAXFCP] ;            /* Xids for calctool canvases. */
  Drawable but_pr[MAXIMAGES] ;      /* Xids for button server images. */
  Drawable corner_pr[MAXIMAGES] ;   /* Xids for button corners. */
  Drawable grey_pr ;
  Drawable menu_pr[MAXIMAGES] ;     /* Xid for menu button glyphs. */
  Pixmap scratch1 ;                 /* First scratch area for images. */
  Pixmap scratch2 ;                 /* Second scratch area for images. */

  GC gc ;                           /* Main drawing graphics context. */
  GC ropgc ;                        /* Graphics context for rops. */
  GC stencilgc ;                    /* Graphics context for stencils. */
  GC svgc ;                         /* Main GC for 1 bit server images. */
  GC svropgc ;                      /* Rop GC for 1 bit server images. */
  XFontStruct *font[MAXFONTS] ;     /* Xlib handles to the fonts. */
  XGCValues gc_val ;                /* To setup graphics context values. */
  XrmDatabase desksetDB ;           /* Deskset resources database. */
  XrmDatabase rDB ;                 /* Combined resources database. */
  char *home ;                      /* Pointer to user's home directory. */
  char *iconfont ;                  /* Font to use for icon label. */
  int gc_flags ;                    /* To set up graphics context flags. */
  int screen ;                      /* Default graphics display screen. */
  unsigned long backgnd ;           /* Default background color. */
  unsigned long foregnd ;           /* Default foreground color. */
  unsigned long gc_mask ;           /* Mask for graphic context values. */
  unsigned long palette[CALC_COLORSIZE] ;  /* Xlib color palette. */

/* Data for holding information about the server's keyboard mapping. */

  int kcmin ;                       /* Minimum keycode. */
  int kcmax ;                       /* Maximum keycode. */
  int keysyms_per_key ;             /* Keysyms per keycode. */
  unsigned char *kparray ;          /* Array indicating if key is on keypad. */

  int cmap_loaded ;                 /* Has the colormap being loaded? */
  int firstpaint ;                  /* TRUE for first valid canvas repaint. */
  int menu_showing ;                /* Set if popup menu visible. */
  int menuval ;                     /* Index to button array at menu time. */
  int mrec[MAXMENUS] ;
  int multi_byte ;                  /* Set TRUE if multi-byte locale. */
  int repaint ;                     /* Set in canvas_repaint when ready. */
} XObject ;

typedef struct Xobject *XVars ;

Notify_value bshow_ascii   P((Panel_item, Event *)) ;
Notify_value canvas_proc   P((Xv_Window, Event *,
                              Notify_arg, Notify_event_type)) ;

Panel_setting tshow_ascii  P((Panel_item, Event *)) ;

void canvas_repaint        P((Canvas, Xv_Window, Display *,
                              Xv_Window, Xv_xrectlist *)) ;
void canvas_resize         P((Canvas, int, int)) ;
void prop_apply            P((Panel_item, Event *)) ;
void prop_defaults         P((Panel_item, Event *)) ;
void prop_reset            P((Panel_item, Event *)) ;
void write_cf_value        P((Panel_item, Event *)) ;


static int event_is_keypad         P((Event *)) ;
static int get_next_event          P((Event *)) ;
static int xview_error_proc        P((Xv_object, Attr_avlist)) ;

static Drawable make_server_image  P((unsigned short *)) ;
static KeySym keypad_keysym        P((Event *)) ;
static Seln_result reply_proc      P((Seln_attribute,
                                      Seln_replier_data *, int)) ;
static XRectangle compress_repaint P((Xv_Window, Xv_xrectlist *)) ;

static void add_but_corner     P((Drawable, enum but_state, enum scale_type,
                                  enum corner_type, int, int)) ;
static void create_aframe      P(()) ;
static void create_cfframe     P(()) ;
static void display_prop_sheet P(()) ;
static void done_proc          P((Frame)) ;
static void func_key_proc      P((char *, Seln_function_buffer *)) ;
static void get_proc           P((Seln_request *)) ;
static void load_colors        P(()) ;
static void menu_done_proc     P((Menu, Xv_opaque)) ;
static void menu_proc          P((Menu, Menu_item)) ;
static void new_cf_value       P((Menu, Menu_item)) ;
static void set_color          P((char *, int, int)) ;

static XVars X ;

static unsigned short grey_image[] = {
#include "images/grey.icon"
} ;

static unsigned short menu_normal_image[] = {
#include "images/menu.normal.icon"
} ;

static unsigned short menu_invert_image[] = {
#include "images/menu.invert.icon"
} ;

static unsigned short menu_stencil_image[] = {
#include "images/menu.stencil.icon"
} ;


Attr_attribute INSTANCE ;
calctool_kframe_objects  *Calctool_kframe ;
calctool_mframe_objects  *Calctool_mframe ;
calctool_Aframe_objects  *Calctool_Aframe ;
calctool_rframe_objects  *Calctool_rframe ;
calctool_CFframe_objects *Calctool_CFframe ;
calctool_Pframe_objects  *Calctool_Pframe ;


int
main(argc, argv)
int argc ;
char **argv ;
{
  char bind_home[MAXPATHLEN] ;

  v = (Vars)  LINT_CAST(calloc(1, sizeof(CalcVars))) ;
  X = (XVars) LINT_CAST(calloc(1, sizeof(XObject))) ;
  X->rank       = SELN_SHELF ;
  X->home       = getenv("HOME") ;
  X->firstpaint = TRUE ;

  ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home) ;
  bindtextdomain(MSGFILE_ERROR, bind_home) ;
  bindtextdomain(MSGFILE_LABEL, bind_home) ;
  bindtextdomain(MSGFILE_MESSAGE, bind_home) ;

  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
          XV_ERROR_PROC,         xview_error_proc,
          XV_USE_LOCALE,         TRUE,
          0) ;

  if (multibyte) X->multi_byte = TRUE ;

  Calctool_kframe  = calctool_kframe_objects_initialize(NULL, NULL) ;
  Calctool_mframe  = calctool_mframe_objects_initialize(NULL,
                                                    Calctool_kframe->kframe) ;
  Calctool_rframe  = calctool_rframe_objects_initialize(NULL,
                                                    Calctool_kframe->kframe) ;
  Calctool_Aframe  = NULL ;
  Calctool_CFframe = NULL ;
  Calctool_Pframe  = NULL ;

  XV_SET(Calctool_kframe->kframe,
#ifdef OW_I18N
/*         WIN_USE_IM,       FALSE,  */
#endif /*OW_I18N*/
         FRAME_NO_CONFIRM, TRUE,
         0) ;
  X->dpy = (Display *) xv_get(Calctool_kframe->kframe, XV_DISPLAY) ;

  do_calctool(argc, argv) ;
  exit(0) ;
/*NOTREACHED*/
}


void
add_3D_corner(fcptype, scale, corner, x, y, width, height, color)
enum fcp_type fcptype ;
enum scale_type scale ;
enum corner_type corner ;
int x, y, width, height, color ;
{
  int s ;
  int dx, dy ;    /* Position of corner piece in the calctool window. */
  int sx, sy ;    /* Position of appropriate corner in corner image. */

/*  Depending upon the current scale and the corner in question, work
 *  out the position of where it's got to go, and where to find it in
 *  the corner server image.
 */

  s = cornerR[(int) scale] ;
  if (corner == TL_CORNER || corner == BL_CORNER) dx = x ;
  else                                            dx = x + width - s ;

  if (corner == TL_CORNER || corner == TR_CORNER) dy = y ;
  else                                            dy = y + height - s ;

  sx = cornerX[(int) scale] ;
  if (corner == TR_CORNER || corner == BR_CORNER) sx += s ;

  sy = cornerY[(int) scale] ;
  if (corner == BL_CORNER || corner == BR_CORNER) sy += s ;

  XCopyArea(X->dpy, X->corner_pr[(int) B_NORMAL], X->scratch1, X->svgc,
            sx, sy, s, s, 0, 0) ;

  X->gc_val.foreground = X->palette[color] ;
  X->gc_val.clip_x_origin = dx ;
  X->gc_val.clip_y_origin = dy ;
  X->gc_val.clip_mask = X->scratch1 ;
  X->gc_val.stipple = X->scratch1 ;
  X->gc_val.ts_x_origin = dx ;
  X->gc_val.ts_y_origin = dy ;
  X->gc_mask = GCForeground | GCClipMask | GCClipXOrigin | GCClipYOrigin |
               GCStipple | GCTileStipXOrigin | GCTileStipYOrigin ;
  XChangeGC(X->dpy, X->stencilgc, X->gc_mask, &X->gc_val) ;
  XFillRectangle(X->dpy, X->xid[(int) fcptype], X->stencilgc,
                 dx, dy, s, s) ;
}


static void
add_but_corner(xid, bstate, scale, corner, width, height)
Drawable xid ;
enum but_state bstate ;
enum scale_type scale ;
enum corner_type corner ;
int width, height ;
{
  int s ;
  int dx, dy ;    /* Position of corner piece in server image. */
  int sx, sy ;    /* Position of appropriate corner in corner image. */

/*  Depending upon the current scale and the corner in question, work
 *  out the position of where it's got to go, and where to find it in
 *  the corner server image.
 */
 
  s = cornerR[(int) scale] ;
  if (corner == TL_CORNER || corner == BL_CORNER) dx = 0 ;
  else                                            dx = width - s ;
 
  if (corner == TL_CORNER || corner == TR_CORNER) dy = 0 ;
  else                                            dy = height - s ;
 
  sx = cornerX[(int) scale] ;
  if (corner == TR_CORNER || corner == BR_CORNER) sx += s ;
 
  sy = cornerY[(int) scale] ;
  if (corner == BL_CORNER || corner == BR_CORNER) sy += s ;
 
  XCopyArea(X->dpy, X->corner_pr[(int) bstate], xid, X->svgc,
            sx, sy, s, s, dx, dy) ;
}


void
beep()
{
  ds_beep(X->dpy) ;
}


/*ARGSUSED*/
Notify_value
bshow_ascii(item, event)
Panel_item item ;
Event *event ;
{
  char *ch ;
  int val ;

  ch = (char *) xv_get(Calctool_Aframe->Api_text, PANEL_VALUE) ;
  val = ch[0] ;
  mpcim(&val, v->MPdisp_val) ;
  show_display(v->MPdisp_val) ;
  return(NOTIFY_DONE) ;
}


Notify_value
canvas_proc(window, event, arg, type)
Xv_Window window ;
Event *event ;
Notify_arg arg ;
Notify_event_type type ;
{
  int id ;
  Notify_value rc ;

  rc = notify_next_event_func(window, (Notify_event) event, arg, type) ;
  if (!v->started) return(rc) ;
  X->cur_event  = event ;
  id = event_id(event) ;
  if (id != KBD_USE && id != KBD_DONE)
    {
           if (window == X->pw[(int) FCP_KEY])  v->curwin = FCP_KEY ;
      else if (window == X->pw[(int) FCP_REG])  v->curwin = FCP_REG ;
      else if (window == X->pw[(int) FCP_MODE]) v->curwin = FCP_MODE ;
    }
  process_event(v->event_type = get_next_event(event)) ;
  return(rc) ;
}


/*ARGSUSED*/
void
canvas_repaint(canvas, window, display, xid, xrects)
Canvas canvas ;             /* Canvas handle */
Xv_Window window ;              /* Pixwin/Paint window */
Display *display ;          /* Server connection */
Xv_Window xid ;             /* X11 window handle */
Xv_xrectlist *xrects ;      /* Damage rectlist */
{
  XRectangle xr ;
  int x, y, width, height ;

  if (!v->started) return ;
  X->repaint = TRUE ;        /* Ready to handle resize events now. */

/*  The window manager does not read the NormalHints property until the
 *  frame comes out of it's withdrawn state. So we set minimal size hints
 *  here, the very first time through.
 */

  if (X->firstpaint == TRUE)
    {
      X->firstpaint = FALSE ;
      XV_SET(Calctool_kframe->kframe,
             FRAME_MIN_SIZE, v->minwidth, v->minheight,
             0) ;
    }

       if (window == X->pw[(int) FCP_KEY])  v->curwin = FCP_KEY ;
  else if (window == X->pw[(int) FCP_REG])  v->curwin = FCP_REG ;
  else if (window == X->pw[(int) FCP_MODE]) v->curwin = FCP_MODE ;

  if (v->curwin == FCP_REG) make_registers() ;
  else
    {
      if (xrects)
        {
          xr     = compress_repaint(xid, xrects) ;
          x      = xr.x ;
          y      = xr.y ;
          width  = xr.width ;
          height = xr.height ;
        }
      else get_frame_size(FCP_KEY, &x, &y, &width, &height) ;

      switch (v->curwin)
        {
          case FCP_KEY  : make_canvas(x, y, width, height, 0) ;
                          break ;
          case FCP_MODE : if (v->modetype != BASIC)
                            make_modewin(x, y, width, height) ;
        }
    }
}


/*ARGSUSED*/
void
canvas_resize(canvas, width, height)
Canvas canvas ;
int width, height ;
{
  int x, y ;

  if (!(v->started && X->repaint)) return ;
  if (canvas == Calctool_kframe->kcanvas)
    {
      if (width != v->twidth || height != v->theight)
        {
          if (width < v->minwidth || height < v->minheight)
            {
              v->scale = S_SMALL ;
              get_frame_size(FCP_KEY, &x, &y, &width, &height) ;
              if (width  < v->minwidth)  width  = v->minwidth ;
              if (height < v->minheight) height = v->minheight ;
              set_frame_size(FCP_KEY, x, y, width, height) ;
            } 

          v->theight = height ;
          v->twidth  = width ;
          do_canvas_resize() ;
          v->theight = (int) xv_get(Calctool_kframe->kcanvas, XV_HEIGHT) ;
          v->twidth  = (int) xv_get(Calctool_kframe->kcanvas, XV_WIDTH) ;
        }
    }
}


/*  Checks for font, width, height, scale, header, geometry, icon image
 *  icon label, reverse video, foreground color and background color.
 */

void
check_args()
{
  char *tempstr ;
  int reverse = 0 ;

  if (defaults_exists("font.name", "Font.Name"))
    {
      tempstr = (char *) defaults_get_string("font.name",
                                             "Font.Name", DEFFONT) ;
      read_str(&v->fontnames[(int) NFONT], tempstr) ;
      read_str(&v->fontnames[(int) SFONT], tempstr) ;
      read_str(&v->fontnames[(int) MFONT], tempstr) ;
      read_str(&v->fontnames[(int) BFONT], tempstr) ;
    }
  if (defaults_exists("icon.font.name", "Icon.Font.Name"))
    {
      tempstr = (char *) defaults_get_string("icon.font.name",
                                             "Icon.Font.Name", DEFFONT) ;
      read_str(&X->iconfont, tempstr) ;
    }
  if (defaults_exists("window.width", "Window.Width"))
    {
      v->iwidth = defaults_get_integer("window.width", "Window.Width", 1) ;
      if (v->iwidth < 0) v->iwidth = -1 ;
    }
  if (defaults_exists("window.height", "Window.Height"))
    {
      v->iheight = defaults_get_integer("window.height", "Window.Height", 1) ;
      if (v->iheight < 0) v->iheight = -1 ;
    }

  if (defaults_exists("window.iconic", "Window.Iconic"))
    v->iconic = defaults_get_boolean("window.iconic", "Window.Iconic", 0) ;

  if (defaults_exists("window.scale", "Window.Scale"))
    {
      tempstr = (char *) defaults_get_string("window.scale",
                                             "Window.Scale", "medium") ;
      get_font_scale(tempstr) ;
      v->isscale = TRUE ;
    }
  if (defaults_exists("window.header", "Window.Header"))
    {
      tempstr = (char *) defaults_get_string("window.header",
                                             "Window.Header", "Calculator") ;
      if (tempstr == NULL || *tempstr == '\0') tempstr = " " ;
      read_str(&v->titleline, tempstr) ;
    }
  if (defaults_exists("window.geometry", "Window.Geometry"))
    {
      tempstr = (char *) defaults_get_string("window.geometry",
                                             "Window.Geometry", "") ;
      ds_get_geometry_size(tempstr, &v->iwidth, &v->iheight) ;
    }
  if (defaults_exists("icon.pixmap", "Icon.Pixmap"))
    {
      tempstr = (char *) defaults_get_string("icon.pixmap",
                                             "Icon.Pixmap", "") ;
      if (tempstr != NULL && *tempstr != '\0' && access(tempstr, R_OK) == 0)
        v->hasicon = TRUE ;
      else
        {
          FPRINTF(stderr, mess[(int) MESS_ICON], v->progname, tempstr) ;
          xv_usage(vstrs[(int) V_CALCTOOL]) ;
          exit(1) ;
        }
    }
  if (defaults_exists("icon.footer", "Icon.Footer"))
    {
      tempstr = (char *) defaults_get_string("icon.footer", "Icon.Footer",
                                             lstrs[(int) L_LCALC]) ;
      if (tempstr == NULL || *tempstr == '\0') tempstr = " " ;
      read_str(&v->iconlabel, tempstr) ;
    }
  if (defaults_exists("window.reverseVideo", "Window.ReverseVideo"))
    {
      reverse = defaults_get_boolean("window.reverseVideo",
                                     "Window.ReverseVideo", 0) ;
    }
  if (defaults_exists("window.color.foreground", "Window.Color.Foreground"))
    {
      tempstr = (char *) defaults_get_string("window.color.foreground",
                                             "Window.Color.Foreground", "") ;
      set_color(tempstr, TRUE, reverse) ;
    }
  if (defaults_exists("window.color.background", "Window.Color.Background"))
    {
      tempstr = (char *) defaults_get_string("window.color.background",
                                             "Window.Color.Background", "") ;
      set_color(tempstr, FALSE, reverse) ;
    }
}


void
check_ow_beep()
{
  char *tempstr ;

  if (defaults_exists("openwindows.beep", "OpenWindows.Beep"))
    {
      tempstr = (char *) defaults_get_string("openwindows.beep",
                                             "OpenWindows.Beep", "") ;
      if (EQUAL(tempstr, "always")) v->beep = TRUE ;
      else                          v->beep = FALSE ;
    }
}


void
color_area(fcptype, x, y, width, height, color)
enum fcp_type fcptype ;
int x, y, width, height, color ;
{
  if (X->menu_showing && !v->iscolor) return ;
  if (v->iscolor) X->gc_val.foreground = X->palette[color] ;
  else
    { 
      if (color == C_WHITE) X->gc_val.foreground = X->backgnd ;
      else                  X->gc_val.foreground = X->foregnd ;
    }
  X->gc_val.function = GXcopy ;
  X->gc_mask         = GCForeground | GCFunction ;
  XChangeGC(X->dpy, X->gc, X->gc_mask, &X->gc_val) ;
  XFillRectangle(X->dpy, X->xid[(int) fcptype], X->gc,
                 x, y, (unsigned int) width, (unsigned int) height) ;
}


static XRectangle
compress_repaint(xid, xrects)
Xv_Window xid ;
Xv_xrectlist *xrects ;
{
  XEvent ev ;
  XRectangle *xr = xrects->rect_array ;
  XRectangle clip ;

  int i ;
  int minx = xr->x ;
  int miny = xr->y ;
  int maxx = minx + xr->width ;
  int maxy = miny + xr->height ;

/* Look through this expose event building the bbox */

  for (i = 1, xr++; i < xrects->count; i++, xr++)
    {
      if (xr->x < minx)                      minx = xr->x ;
      if (xr->y < miny)                      miny = xr->y ;
      if ((int) (xr->x + xr->width) > maxx)  maxx = xr->x + xr->width ;
      if ((int) (xr->y + xr->height) > maxy) maxy = xr->y + xr->height ;
    }
 
  XSync(X->dpy, 0) ;
 
/* Look through pending expose events building the bbox */
 
  while (XPending(X->dpy) && (XPeekEvent(X->dpy, &ev),
         (ev.type == Expose && ev.xany.window == xid)))
    {
      XNextEvent(X->dpy, &ev) ;
      if (ev.xexpose.x < minx) minx = ev.xexpose.x ;
      if (ev.xexpose.y < miny) miny = ev.xexpose.y ;
      if ((int) (ev.xexpose.x + ev.xexpose.width) > maxx)
        maxx = ev.xexpose.x + ev.xexpose.width ;
      if ((int) (ev.xexpose.y + ev.xexpose.height) > maxy)
        maxy = ev.xexpose.y + ev.xexpose.height ;
    }

/* Confine drawing to the extent of the damage */

  clip.x      = minx ;
  clip.y      = miny ;
  clip.width  = maxx - minx ;
  clip.height = maxy - miny ;
  return(clip) ;
}


static void
create_aframe()  /* Create auxiliary frame for ASC key. */
{
  Calctool_Aframe = calctool_Aframe_objects_initialize(NULL,
                                                  Calctool_kframe->kframe) ;
  XV_SET(Calctool_Aframe->Aframe,
#ifdef OW_I18N
/*         WIN_USE_IM, TRUE,  */
#endif /*OW_I18N*/
         XV_SHOW,    FALSE,
         XV_X,       v->twidth / 2,
         XV_Y,       v->theight + v->ndisplay + 15,
         0) ;

  XV_SET(Calctool_Aframe->Api_text,
         PANEL_NOTIFY_STRING, "\n\r",
         0) ;

  DS_ADD_HELP(Calctool_Aframe->Aframe,   hstrs[(int) H_AFRAME]) ;
  DS_ADD_HELP(Calctool_Aframe->Apanel,   hstrs[(int) H_APANEL]) ;
  DS_ADD_HELP(Calctool_Aframe->Api_text, hstrs[(int) H_ATEXT]) ;
  DS_ADD_HELP(Calctool_Aframe->Api_but,  hstrs[(int) H_ABUT]) ;
  DS_CENTER_ITEMS(Calctool_Aframe->Apanel, -1,
                         Calctool_Aframe->Api_but,
                         0) ;
}


static void
create_cfframe()    /* Create auxiliary frame for CON/FUN key. */
{
  Calctool_CFframe = calctool_CFframe_objects_initialize(NULL,
                                                 Calctool_kframe->kframe) ;
  XV_SET(Calctool_CFframe->CFframe,
#ifdef OW_I18N
/*         WIN_USE_IM, TRUE,  */
#endif /*OW_I18N*/
         XV_X,       0,
         XV_Y,       v->theight + v->ndisplay + 25,
         0) ;

  XV_SET(Calctool_CFframe->CFpi_fbut,
         XV_Y, (int) xv_get(Calctool_CFframe->CFpi_cbut, XV_Y),
         0) ;

  DS_ADD_HELP(Calctool_CFframe->CFframe,     hstrs[(int) H_CFFRAME]) ;
  DS_ADD_HELP(Calctool_CFframe->CFpanel,     hstrs[(int) H_CFPANEL]) ;
  DS_ADD_HELP(Calctool_CFframe->CFpi_cftext, hstrs[(int) H_CFNO]) ;
  DS_ADD_HELP(Calctool_CFframe->CFpi_dtext,  hstrs[(int) H_CFDESC]) ;
  DS_ADD_HELP(Calctool_CFframe->CFpi_vtext,  hstrs[(int) H_CFVAL]) ;
  DS_ADD_HELP(Calctool_CFframe->CFpi_cbut,   hstrs[(int) H_CFCBUT]) ;
  DS_ADD_HELP(Calctool_CFframe->CFpi_fbut,   hstrs[(int) H_CFFBUT]) ;

  DS_CENTER_ITEMS(Calctool_CFframe->CFpanel, -1,
                  Calctool_CFframe->CFpi_cbut,
                  0) ;
  DS_CENTER_ITEMS(Calctool_CFframe->CFpanel, -1,
                  Calctool_CFframe->CFpi_fbut,
                  0) ;
}


void
create_con_fun_menu(mtype, n)    /* Create/update constant/function menu. */
enum menu_type mtype ;
int n ;
{
  Frame frame ;
  Menu_item mi ;
  char *mstr ;
  int i, insert_pt, invalid ;

  if (X->menus[(int) mtype] == NULL)
    {
      X->menus[(int) mtype] = xv_create(XV_NULL,     MENU_COMMAND_MENU,
                                MENU_TITLE_ITEM,     cmenus[(int) mtype].title,
                                MENU_GEN_PIN_WINDOW, Calctool_kframe->kframe,
                                                     cmenus[(int) mtype].title,
                                MENU_NOTIFY_PROC,    menu_proc,
                                MENU_DONE_PROC,      menu_done_proc,
                                0) ;

      mstr = (mtype == M_CON) ? vstrs[(int) V_CONWNAME]
                              : vstrs[(int) V_FUNWNAME] ;
      XV_SET(X->menus[(int) mtype], MENU_ACTION_ITEM, mstr, new_cf_value, 0) ;
    }

  X->mrec[(int) mtype] = n ;
  insert_pt = 2 ;
  for (i = 0; i < cmenus[(int) mtype].total; i++)
    {
      invalid = 0 ;
      mstr = (mtype == M_CON) ? v->con_names[i] : v->fun_names[i] ;
      if (!strlen(mstr)) invalid = 1 ;
      if (!invalid)
        {
          mi = xv_create(XV_NULL, MENUITEM,
                 MENU_STRING,     mstr,
                 MENU_VALUE,  menu_entries[cmenus[(int) mtype].index + i],
                 0) ;
          XV_SET(X->menus[(int) mtype], MENU_INSERT, insert_pt, mi, 0) ;
          insert_pt++ ;
        }
    }
  XV_SET(X->menus[(int) mtype], MENU_DEFAULT, cmenus[(int) mtype].defval, 0) ;
}


void
create_menu(mtype, n)        /* Create popup menu for right button press. */
enum menu_type mtype ;
int n ;
{
  Menu menu ;
  Menu_item mi ;
  int i ;

  if ((mtype != M_CON && mtype != M_FUN) &&
      X->menus[(int) mtype] != NULL) { /* Already created? */
  	X->mrec[(int) mtype]  = n ;   /* may have changed from left to right */
	return;
  }       

  switch (mtype)
    {
      case M_ACC   : menu = calctool_acc_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_BASE  : menu = calctool_base_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_EXCH  : menu = calctool_exch_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_LSHF  : menu = calctool_lshift_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_MODE  : menu = calctool_mode_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_NUM   : menu = calctool_disp_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_RCL   : menu = calctool_rcl_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_RSHF  : menu = calctool_rshift_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_STO   : menu = calctool_sto_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_TRIG  : menu = calctool_trig_menu_create((caddr_t) INSTANCE,
                                                      Calctool_kframe->kframe) ;
                     break ;
      case M_PROPS : menu = calctool_props_menu_create((caddr_t) INSTANCE,
                                                        NULL) ;
                     break ;
      case M_CON   :
      case M_FUN   : if (X->menus[(int) mtype] == NULL)
                       create_con_fun_menu(mtype, n) ;
                     return ;
    }

  X->mrec[(int) mtype]  = n ;
  X->menus[(int) mtype] = menu ;
  DS_ADD_HELP(X->menus[(int) mtype], hstrs[(int) mtype]) ;
  XV_SET(X->menus[(int) mtype],
         MENU_NOTIFY_PROC, menu_proc,
         MENU_DONE_PROC,   menu_done_proc,
         0) ;

  for (i = 0; i < cmenus[(int) mtype].total; i++)
    {
      mi = (Menu_item) xv_get(X->menus[(int) mtype], MENU_NTH_ITEM, i+2) ;
      XV_SET(mi,
             MENU_VALUE, menu_entries[cmenus[(int) mtype].index + i],
             0) ;
    }

  XV_SET(X->menus[(int) mtype], MENU_DEFAULT, cmenus[(int) mtype].defval, 0) ;
}


static void
display_prop_sheet()
{
  init_options() ;
  if (xv_get(Calctool_Pframe->Pframe, XV_SHOW) == FALSE)
    {
      ds_position_popup(Calctool_kframe->kframe,
                        Calctool_Pframe->Pframe, DS_POPUP_LOR) ;
      XV_SET(Calctool_Pframe->Pframe, XV_SHOW, TRUE, 0) ;
    }
}


int
do_menu(fcptype, n, menutype)     /* Popup appropriate menu and get value. */
enum fcp_type fcptype ;
int n ;
enum menu_type menutype ;
{
  X->menu_showing = 1 ;
  X->menuval = n ;
  create_menu(menutype, n) ;
  menu_show(X->menus[(int) menutype],
            X->pw[(int) fcptype], X->cur_event, 0) ;
  return(0) ;
}


static void
done_proc(f)
Frame f ;
{
       if (f == Calctool_rframe->rframe) v->rstate = 0 ;
  else if (f == Calctool_mframe->mframe)
    {
      v->modetype = BASIC ;
      set_item(MODEITEM, mode_str[(int) v->modetype]) ;
    }
  XV_SET(f, XV_SHOW, FALSE, 0) ;
}


void
draw_image(fcptype, x, y, color, state)
enum fcp_type fcptype ;
int color, x, y ;
enum but_state state ;
{
  if (X->menu_showing && !v->iscolor) return ;
  X->gc_mask = GCForeground | GCStipple |
               GCTileStipXOrigin | GCTileStipYOrigin | GCFillStyle ;
  if (v->iscolor) X->gc_val.foreground = X->palette[color] ;
  else            X->gc_val.foreground = X->foregnd ;
  X->gc_val.stipple = X->but_pr[(int) state] ;
  X->gc_val.ts_x_origin = x ;
  X->gc_val.ts_y_origin = y ;
  X->gc_val.fill_style = FillStippled ;
  XChangeGC(X->dpy, X->ropgc, X->gc_mask, &X->gc_val) ;
  XFillRectangle(X->dpy, X->xid[(int) fcptype], X->ropgc,
                 x, y, v->bwidth, v->bheight) ;
}


void
draw_line(fcptype, x1, y1, x2, y2, color)
enum fcp_type fcptype ;
int x1, y1, x2, y2, color ;
{
  if (X->menu_showing && !v->iscolor) return ;
  if (v->iscolor) X->gc_val.foreground = X->palette[color] ;
  else
    { 
      if (color == C_WHITE) X->gc_val.foreground = X->backgnd ;
      else                  X->gc_val.foreground = X->foregnd ;
    }
  X->gc_val.function = GXcopy ;
  XChangeGC(X->dpy, X->gc, GCForeground | GCFunction, &X->gc_val) ;
  XDrawLine(X->dpy, X->xid[(int) fcptype], X->gc, x1, y1, x2, y2) ;
}


void
draw_menu_image(fcptype, x, y, state)
enum fcp_type fcptype ;
int x, y ;
enum but_state state ;
{
  int m ;      /* Dimension of the menu glyph at this scale. */

  if (X->menu_showing && !v->iscolor) return ;

  m = msizes[(int) v->scale] ;
  XCopyArea(X->dpy, X->menu_pr[(int) state], X->scratch1, X->svgc,
            cornerX[(int) v->scale], cornerY[(int) v->scale], m, m, 0, 0) ;

  X->gc_mask = GCForeground | GCStipple |
               GCTileStipXOrigin | GCTileStipYOrigin | GCFillStyle ;
  X->gc_val.foreground  = X->foregnd ;
  X->gc_val.stipple     = X->scratch1 ;
  X->gc_val.ts_x_origin = x ;
  X->gc_val.ts_y_origin = y ;
  X->gc_val.fill_style  = FillStippled ;
  XChangeGC(X->dpy, X->ropgc, X->gc_mask, &X->gc_val) ;
  XFillRectangle(X->dpy, X->xid[(int) fcptype], X->ropgc, x, y, m, m) ;
}


void
draw_menu_stencil(fcptype, x, y, color, state)
enum fcp_type fcptype ;
int color, x, y ;
enum but_state state ;
{
  int sx, sy ;     /* Position of glyph with menu server image. */
  int m ;          /* Dimension of the menu glyph at this scale. */

  if (X->menu_showing && !v->iscolor) return ;

  m  = msizes[(int) v->scale] ;
  sx = cornerX[(int) v->scale] ;
  sy = cornerY[(int) v->scale] ;
  XCopyArea(X->dpy, X->menu_pr[(int) state], X->scratch1, X->svgc,
            sx, sy, m, m, 0, 0) ;
  XCopyArea(X->dpy, X->menu_pr[(int) B_STENCIL], X->scratch2, X->svgc,
            sx, sy, m, m, 0, 0) ;

  if (v->iscolor) X->gc_val.foreground = X->palette[color] ;
  else
    {
      if (color == C_BLACK) X->gc_val.foreground = X->foregnd ;
      else                  X->gc_val.foreground = X->backgnd ;
    }
  X->gc_val.clip_x_origin = x ;
  X->gc_val.clip_y_origin = y ;
  X->gc_val.clip_mask     = X->scratch1 ;
  X->gc_val.stipple       = X->scratch2 ;
  X->gc_val.ts_x_origin   = x ;
  X->gc_val.ts_y_origin   = y ;
  X->gc_mask = GCForeground | GCClipMask | GCClipXOrigin | GCClipYOrigin |
               GCStipple | GCTileStipXOrigin | GCTileStipYOrigin ;
  XChangeGC(X->dpy, X->stencilgc, X->gc_mask, &X->gc_val) ;
  XFillRectangle(X->dpy, X->xid[(int) fcptype], X->stencilgc, x, y, m, m) ;
}


void
draw_stencil(fcptype, x, y, color, state)
enum fcp_type fcptype ;
int color, x, y ;
enum but_state state ;
{
  if (X->menu_showing && !v->iscolor) return ;
  if (v->iscolor) X->gc_val.foreground = X->palette[color] ;
  else            X->gc_val.foreground = X->foregnd ;
  X->gc_val.clip_x_origin = x ;
  X->gc_val.clip_y_origin = y ;
  X->gc_val.clip_mask     = X->but_pr[(int) B_STENCIL] ;
  X->gc_val.stipple       = X->but_pr[(int) state] ;
  X->gc_val.ts_x_origin   = x ;
  X->gc_val.ts_y_origin   = y ;
  X->gc_mask = GCForeground | GCClipMask | GCClipXOrigin | GCClipYOrigin |
               GCStipple | GCTileStipXOrigin | GCTileStipYOrigin ;
  XChangeGC(X->dpy, X->stencilgc, X->gc_mask, &X->gc_val) ;
  XFillRectangle(X->dpy, X->xid[(int) fcptype], X->stencilgc,
                 x, y, v->bwidth, v->bheight) ;
}


void
draw_text(x, y, fcptype, fonttype, color, str)
enum font_type fonttype ;
enum fcp_type fcptype ;
int x, y, color ;
char *str ;
{
  if (X->menu_showing && !v->iscolor) return ;
  if (v->iscolor) X->gc_val.foreground = X->palette[color] ;
  else
    { 
      if (color == C_WHITE) X->gc_val.foreground = X->backgnd ;
      else                  X->gc_val.foreground = X->foregnd ;
    }
  X->gc_val.font     = X->font[(int) fonttype]->fid ;
  X->gc_val.function = GXcopy ;
  XChangeGC(X->dpy, X->gc, GCFont | GCForeground | GCFunction, &X->gc_val) ;

#ifdef OW_I18N
  if (X->multi_byte)
    XmbDrawString(X->dpy, X->xid[(int) fcptype], X->fontset, X->gc,
                  x, y + v->fascent[(int) fonttype], str, strlen(str)) ;
  else
#endif /*OW_I18N*/
  XDrawString(X->dpy, X->xid[(int) fcptype], X->gc,
              x, y + v->fascent[(int) fonttype], str, strlen(str)) ;
}


/*  Tell whether an event is a keyboard event from a keypad key. This function
 *  looks at the raw X event and uses information from Xlib to make this
 *  determination. This function is sometimes necessary because there are
 *  often several keysyms on a keypad key, and XView doesn't always give the
 *  "right" one.
 */

static int
event_is_keypad(event)
Event *event ;
{
  XEvent *xk = event_xevent(event) ;

  if (xk->type != KeyPress && xk->type != KeyRelease) return(0) ;
  return(X->kparray[xk->xkey.keycode - X->kcmin] > 0) ;
}


Notify_value
frame_interpose(frame, event, arg, type)
Frame frame ;
Event *event ;
Notify_arg arg ;
Notify_event_type type ;
{
  int action, height, width ;
  Notify_value rc ;
  Rect *r ;

  rc = notify_next_event_func(frame, (Notify_event) event, arg, type) ;
  if (!v->started) return(rc) ;
  action = event_action(event) ;

  if (frame == Calctool_rframe->rframe)
    {
      if (event_id(event) == WIN_MAP_NOTIFY) make_registers() ;
      return(rc) ;
    }

  if (event_id(event) == WIN_RESIZE)
    {
      r = (Rect *) xv_get(frame, FRAME_OPEN_RECT) ;
      XV_SET(Calctool_kframe->kcanvas,
             XV_WIDTH,  r->r_width,
             XV_HEIGHT, r->r_height,
             0) ;
      canvas_resize(Calctool_kframe->kcanvas, r->r_width, r->r_height) ;
    }
  else if (action == ACTION_CLOSE) v->iconic = TRUE ;
  else if (action == ACTION_OPEN)
    {
      if (v->rstate)            win_display(FCP_REG,  TRUE) ;
      if (v->modetype != BASIC) win_display(FCP_MODE, TRUE) ;
      v->iconic = FALSE ;
    }
  return(rc) ;
}


/*ARGSUSED*/
static void
func_key_proc(client_data, args)
char *client_data ;
Seln_function_buffer *args ;
{
  get_display() ;
}


void
get_but_corners(bstate, data)    /* Load calctool button corners. */
enum but_state bstate ;
unsigned short data[] ;
{
  if (X->corner_sv[(int) bstate] != NULL)
    xv_destroy_safe(X->corner_sv[(int) bstate]) ;
  X->corner_sv[(int) bstate] = xv_create(XV_NULL,           SERVER_IMAGE,
                                         XV_WIDTH,          64,
                                         XV_HEIGHT,         64,
                                         SERVER_IMAGE_BITS, data,
                                         0) ;
  X->corner_pr[(int) bstate] = (Drawable) xv_get(X->corner_sv[(int) bstate],
                                                 XV_XID) ;
}


char *
get_def_menu_str(menutype)
enum menu_type menutype ;
{
  Menu_item item ;

  item = (Menu_item) xv_get(X->menus[(int) menutype], MENU_DEFAULT_ITEM) ;
  if (!item) return(NULL) ;
  else return((char *) xv_get(item, MENU_STRING)) ;
}


void
get_display()     /* The GET function key has been pressed. */
{
  char selstr[MAXLINE] ;  /* Display value or selected portion thereof. */

  int i, len ;
  int sellen = 0 ;        /* Length of display value (or selected portion). */

  if (seln_acquire(X->sel_client, SELN_SHELF) == SELN_SHELF)
    {
      if (v->shelf != NULL) free(v->shelf) ;
      if (v->histart != -1)                        /* Portion selected? */
        {
          len = strlen(v->display) ;
          for (i = 0; i < len; i++)
            if (v->disp_state[i] == TRUE)
              selstr[sellen++] = v->display[i] ;
          selstr[sellen] = '\0' ;
        }
      else
        {
          sellen = strlen(v->display) ;
          STRCPY(selstr, v->display) ;
        }
      v->shelf = malloc((unsigned int) sellen) ;
      STRCPY(v->shelf, selstr) ;     /* Safely keep copy of display. */
    }
}


get_font(size, ftype)
int size ;
enum font_type ftype ;
{
  int f ;
 
  f = (int) ftype ;
  if (X->font[f] != NULL) XUnloadFont(X->dpy, X->font[f]->fid) ;
  if (!(X->font[f] = ds_get_font(X->dpy, v->fontnames[f], size)))
    {
      FPRINTF(stderr, mess[(int) MESS_FONT], v->progname) ;
      exit(1) ;
    }

  v->fascent[f] = X->font[f]->max_bounds.ascent ;
  v->fheight[f] = X->font[f]->max_bounds.ascent +
                  X->font[f]->max_bounds.descent ;
  v->fwidth[f]  = X->font[f]->max_bounds.rbearing -
                  X->font[f]->min_bounds.lbearing ;
}


void
get_frame_size(fcptype, x, y, w, h)
enum fcp_type fcptype ;
int *x, *y, *w, *h ;
{
  Frame f ;

       if (fcptype == FCP_KEY)  f = Calctool_kframe->kframe ;
  else if (fcptype == FCP_REG)  f = Calctool_rframe->rframe ;
  else if (fcptype == FCP_MODE) f = Calctool_mframe->mframe ;
  DS_GET_FRAME_SIZE(f, x, y, w, h) ;
}


int
get_menu_def(n)    /* Returns value of default menu item. */
int n ;
{
  enum menu_type menutype ;
  Menu_item item ;

  menutype = button_mtype(n) ;
  create_menu(menutype, n) ;
  item = (Menu_item) xv_get(X->menus[(int) menutype], MENU_DEFAULT_ITEM) ;
  if (!item) return(-1) ;
  else return((int) xv_get(item, MENU_VALUE)) ;
}


int
get_menu_pos(menutype, n)     /* Returns position of default menu item. */
enum menu_type menutype ;
int n ;
{
  create_menu(menutype, n) ;
  return((int) xv_get(X->menus[(int) menutype], MENU_DEFAULT)) ;
}


static int
get_next_event(event)
Event *event ;
{
  int down, nextc, up ;
  short ev_action;

  down      = event_is_down(event) ;
  nextc     = event_id(event) ;
  up        = event_is_up(event) ;
  ev_action = event_action(event) ;
  v->curx   = event_x(event) ;
  v->cury   = event_y(event) ;
  v->sec    = event->ie_time.tv_sec ;
  v->usec   = event->ie_time.tv_usec ;

  if (event_is_keypad(event))
    {
      switch (keypad_keysym(event))
        {
          case XK_KP_0	      : v->cur_ch = '0' ;
                                break ;
          case XK_KP_1        : v->cur_ch = '1' ;
                                break ;
          case XK_KP_2        : v->cur_ch = '2' ;
                                break ;
          case XK_KP_3        : v->cur_ch = '3' ;
                                break ;
          case XK_KP_4        : v->cur_ch = '4' ;
                                break ;
          case XK_KP_5        : v->cur_ch = '5' ;
                                break ;
          case XK_KP_6        : v->cur_ch = '6' ;
                                break ;
          case XK_KP_7        : v->cur_ch = '7' ;
                                break ;
          case XK_KP_8        : v->cur_ch = '8' ;
                                break ;
          case XK_KP_9        : v->cur_ch = '9' ;
                                break ;
          case XK_KP_Add      : v->cur_ch = '+' ;
                                break ;
          case XK_KP_Subtract : v->cur_ch = '-' ;
                                break ;
          case XK_KP_Multiply : v->cur_ch = 'x' ;
                                break ;
          case XK_KP_Divide   : v->cur_ch = '/' ;
                                break ;
          case XK_KP_Equal    :
          case XK_KP_Enter    : v->cur_ch = '=' ;
                                break ;
          case XK_KP_Decimal  : v->cur_ch = '.' ;
        }
           if (down) return(KEYBOARD_DOWN) ;
      else if (up)   return(KEYBOARD_UP) ;
    }
  else if (IS_KEY(nextc, KEY_BSP) || IS_KEY(nextc, KEY_CLR))
    {
      v->cur_ch = nextc ;       /* Delete and Back Space keys. */
           if (down) return(KEYBOARD_DOWN) ;
      else if (up)   return(KEYBOARD_UP) ;
    }
  else if (ev_action == ACTION_PROPS && down) display_prop_sheet() ;

  if (event_is_button(event))
         if (down && nextc == MS_LEFT)   return(LEFT_DOWN) ;
    else if (down && nextc == MS_MIDDLE) return(MIDDLE_DOWN) ;
    else if (down && nextc == MS_RIGHT)  return(RIGHT_DOWN) ;
    else if (up   && nextc == MS_LEFT)   return(LEFT_UP) ;
    else if (up   && nextc == MS_MIDDLE) return(MIDDLE_UP) ;
    else if (up   && nextc == MS_RIGHT)  return(RIGHT_UP) ;

  if (event_is_ascii(event))
    {

/*  If this is a '*' or Return key press, then map to their better known
 *  equivalents, so that button highlighting works correctly.
 */

           if (nextc == CTL('m')) nextc = KEY_EQ ;
      else if (nextc == '*')      nextc = KEY_MUL ;

/*  All the rest of the ASCII characters. */
 
      v->cur_ch = nextc ;
           if (down) return(KEYBOARD_DOWN) ;
      else if (up)   return(KEYBOARD_UP) ;
    }
 
       if (ev_action == ACTION_GO_LINE_BACKWARD)
    {
      v->cur_ch = KEY_ASC ;                           /* Asc. */
           if (down) return(KEYBOARD_DOWN) ;
      else if (up)   return(KEYBOARD_UP) ;
    }
  else if (ev_action == ACTION_GO_CHAR_FORWARD)
    {
      v->cur_ch = KEY_FRAC ;                          /* Frac. */
           if (down) return(KEYBOARD_DOWN) ;
      else if (up)   return(KEYBOARD_UP) ; 
    }
  else if (ev_action == ACTION_ERASE_LINE_BACKWARD)
    {
      v->cur_ch = KEY_ABS ;                           /* Abs. */
           if (down) return(KEYBOARD_DOWN) ;
      else if (up)   return(KEYBOARD_UP) ; 
    }

  if (nextc == LOC_DRAG)            return(MOUSE_DRAGGING);
  if (((nextc == KEY_TOP(1)) || ev_action == ACTION_HELP) && up)
    return(SHOWHELP) ;
  if ((nextc == KEY_LEFT(6)) && up) return(PUT_ON_SHELF) ;
  if ((nextc == KEY_LEFT(10)) && up) return(PUT_ON_SHELF) ;
  if ((nextc == KEY_LEFT(8)) && up) return(TAKE_FROM_SHELF) ;
  return(LASTEVENTPLUSONE) ;
}


static void
get_proc(buffer)
Seln_request *buffer ;
{
  v->issel = 0 ;
  if (*buffer->requester.context == 0)
    {
      if (buffer == (Seln_request *) NULL ||
          *((Seln_attribute *) buffer->data) != SELN_REQ_CONTENTS_ASCII)
        return ;
      v->selection = buffer->data + sizeof(Seln_attribute) ;
      *buffer->requester.context = 1 ;
    }
  else v->selection = buffer->data ;
  v->issel = 1 ;
}


char *
get_resource(rtype)      /* Get calctool resource from merged database. */
enum res_type rtype ;
{
  char str[MAXLINE] ;

  STRCPY(str, calc_res[(int) rtype]) ;
  return(ds_get_resource(X->rDB, v->appname, str)) ;
}


int
get_strwidth(ftype, str)    /* Get width in pixels of string value. */
enum font_type ftype ;
char *str ;
{
#ifdef OW_I18N
  Font_string_dims dims ;

  if (X->multi_byte)
    {
      if (X->pf == NULL) return(0) ;
      XV_GET(X->pf, FONT_STRING_DIMS, str, &dims) ;
      return(dims.width) ;
    }
#endif /*OW_I18N*/
  return(ds_get_strwidth(X->font[(int) ftype], str)) ;
}


void
grey_area(fcptype, x, y, width, height)
enum fcp_type fcptype ;
int x, y, width, height ;
{
  if (X->menu_showing && !v->iscolor) return ;
  X->gc_mask = GCStipple | GCTileStipXOrigin | GCTileStipYOrigin | GCFillStyle ;
  X->gc_val.stipple     = X->grey_pr ;
  X->gc_val.ts_x_origin = x ;
  X->gc_val.ts_y_origin = y ;
  X->gc_val.fill_style  = (v->iscolor) ? FillStippled : FillOpaqueStippled ;
  XChangeGC(X->dpy, X->ropgc, X->gc_mask, &X->gc_val) ;
  XFillRectangle(X->dpy, X->xid[(int) fcptype], X->ropgc, x, y, width, height) ;
}


void
handle_selection()  /* Handle the GET function key being pressed. */
{
  char context = 0 ;

  X->holder = seln_inquire(X->rank) ;
  if (X->holder.state == SELN_NONE) return ;
  SELN_QUERY(&X->holder, get_proc, &context, SELN_REQ_CONTENTS_ASCII, 0, 0) ;
}


void
init_options()
{
  Font_string_dims dims ;
  int longest = 0 ;         /* Longest panel choice label in pixels. */
  Xv_Font pf ;

  if (Calctool_Pframe == 0)
    {
      Calctool_Pframe = calctool_Pframe_objects_initialize(NULL,
                                                  Calctool_kframe->kframe) ;

      DS_ADD_HELP(Calctool_Pframe->Pframe,      hstrs[(int) H_PFRAME]) ;
      DS_ADD_HELP(Calctool_Pframe->Ppanel,      hstrs[(int) H_PPANEL]) ;
      DS_ADD_HELP(Calctool_Pframe->Pappearance, hstrs[(int) H_APPEARANCE]) ;
      DS_ADD_HELP(Calctool_Pframe->Pdisplay,    hstrs[(int) H_DISPLAY]) ;
      DS_ADD_HELP(Calctool_Pframe->Pstyle,      hstrs[(int) H_STYLE]) ;
      DS_ADD_HELP(Calctool_Pframe->Papply,      hstrs[(int) H_APPLY]) ;
      DS_ADD_HELP(Calctool_Pframe->Pdefaults,   hstrs[(int) H_DEF]) ;
      DS_ADD_HELP(Calctool_Pframe->Preset,      hstrs[(int) H_RESET]) ;
    }

  XV_SET(Calctool_Pframe->Pappearance, PANEL_VALUE, v->is_3D,      0) ;
  XV_SET(Calctool_Pframe->Pdisplay,    PANEL_VALUE, v->monochrome, 0) ;
  XV_SET(Calctool_Pframe->Pstyle,      PANEL_VALUE, v->righthand,  0) ;
}


char *MSGFILE_ERROR   = "SUNW_DESKSET_CALCTOOL_ERR" ;
char *MSGFILE_LABEL   = "SUNW_DESKSET_CALCTOOL_LABEL" ;
char *MSGFILE_MESSAGE = "SUNW_DESKSET_CALCTOOL_MSG" ;


int
is_dblclick()
{
  static int time_threshold ;
  static int dist_threshold ;
  static short first_time = TRUE ;
         short ret_value  = FALSE ;
  int delta_time ;
  int delta_x, delta_y ;

/* First time this is called init the thresholds */

  if (first_time)
    {

/* Get time threshold in milliseconds */

      time_threshold  = 100 *
                        defaults_get_integer("OpenWindows.MultiClickTimeout",
                                             "OpenWindows.MultiClickTimeout",
                                             4) ;
      dist_threshold  = defaults_get_integer("OpenWindows.DragThreshold",
                                             "OpenWindows.DragThreshold", 4) ;
      
      first_time      = FALSE ;
    } 
      
  delta_time  = (v->sec - v->old_sec) * 1000 ;
  delta_time += v->usec     / 1000 ;
  delta_time -= v->old_usec / 1000 ;

/* Is the time within bounds? */

  if (delta_time <= time_threshold)
    {

/* Check to see if the distance is ok */

      delta_x = (v->oldx > v->curx ? v->oldx - v->curx : v->curx - v->oldx) ;
      delta_y = (v->oldy > v->cury ? v->oldy - v->cury : v->cury - v->oldy) ;

      if (delta_x <= dist_threshold && delta_y <= dist_threshold)
        ret_value = TRUE ;
    }
  return(ret_value) ;
}


/*  Get information about the keyboard mappings. Determine which keys are
 *  keypad.
 */

void
key_init()
{
  int i, j ;
  KeySym *tmp ;
  KeySym ks ;

  XDisplayKeycodes(X->dpy, &X->kcmin, &X->kcmax) ;
  tmp = XGetKeyboardMapping(X->dpy, X->kcmin, 1, &X->keysyms_per_key) ;
  XFree((char *) tmp) ;

  X->kparray = (unsigned char *) malloc(X->kcmax - X->kcmin + 1) ;

/*  For each key, run through its list of keysyms.  If this keysym is a
 *  keypad keysym, we know this key is on the keypad.  Mark it as such in
 *  kparray[].
 */

  for (i = X->kcmin; i <= X->kcmax; ++i)
    {
      X->kparray[i - X->kcmin] = 0 ;
      for (j = 0; j < X->keysyms_per_key; ++j)
        {
          ks = XKeycodeToKeysym(X->dpy, i, j) ;
          if (IsKeypadKey(ks))
            {
              X->kparray[i - X->kcmin] = 1 ;
              break ;
            }
        }    
    }    
}


/* Given a keyboard event from the keypad, return the KP_whatever keysym
 * corresponding to the key in the event.  If no keypad keysym can be found,
 * returns NoSymbol.
 */

static KeySym
keypad_keysym(event)
Event *event ;
{
  int i ;
  int keycode = event_xevent(event)->xkey.keycode ;
  KeySym ks, save_ks=-1;

  for (i = 0; i < X->keysyms_per_key; ++i)
    {
      ks = XKeycodeToKeysym(X->dpy, keycode, i) ;
      if (IsKeypadKey(ks)) save_ks = ks;
    }
  if (save_ks != -1)
	return save_ks;
  return NoSymbol;
}


static void
load_colors()      /* Create and load calctool color map. */
{
  Cms cms ;
  Colormap def_cmap ;
  XColor ccol ;
  int i, numcolors ;

  if (v->iscolor)
    {
      cms        = (Cms)      xv_get(Calctool_kframe->kframe, WIN_CMS) ;
      def_cmap   = (Colormap) xv_get(cms, CMS_CMAP_ID) ;
      ccol.flags = DoRed | DoGreen | DoBlue ;
      numcolors  = 0 ;
      for (i = 0; i < CALC_COLORSIZE; i++)
        {
          if (v->colstr[i] == NULL ||
              (XParseColor(X->dpy, def_cmap, v->colstr[i], &ccol) == 0))
            {
              ccol.red   = (unsigned short) (v->rcols[i] << 8) ;
              ccol.green = (unsigned short) (v->gcols[i] << 8) ;
              ccol.blue  = (unsigned short) (v->bcols[i] << 8) ;
            }
          if (XAllocColor(X->dpy, def_cmap, &ccol) == True)
            X->palette[numcolors++] = ccol.pixel ;
        }
      if (numcolors < CALC_COLORSIZE)
        {
          if (set_min_colors() == FALSE)
            {
              FPRINTF(stderr, mess[(int) MESS_COLOR], v->progname) ;
              FPRINTF(stderr, mess[(int) MESS_MONO]) ;
              v->iscolor = 0 ;
            }
          else X->cmap_loaded = 1 ;
        }
      else X->cmap_loaded = 1 ;
    }
}


void
load_deskset_defs()     /* Load current deskset resource database. */
{
  if (X->desksetDB != NULL) XrmDestroyDatabase(X->desksetDB) ;
  X->desksetDB = ds_load_deskset_defs() ;
}


void
load_resources()        /* Load combined X resources databases. */
{ 
  X->rDB = ds_load_resources(X->dpy) ;
}


void
make_button(bstate, scale, width, height)    /* Generate a calctool button. */
enum but_state bstate ;
enum scale_type scale ;
int width, height ;
{
  int b ;                 /* The current button state. */
  int c ;                 /* The radius of a corner piece at this scale. */
  Drawable xid ;          /* Xid for the generate calctool button. */

  b = (int) bstate ;
  if (X->svbut_pr[b] != NULL) xv_destroy_safe(X->svbut_pr[b]) ;
  X->svbut_pr[b] = (Server_image) xv_create(XV_NULL,   SERVER_IMAGE,
                                            XV_WIDTH,  width,
                                            XV_HEIGHT, height,
                                            0) ;
  X->but_pr[b] = xid = (Drawable) xv_get(X->svbut_pr[b], XV_XID) ;
 
/* Create graphics contexts for working with 1bit deep server images. */
 
  if (X->svgc == NULL)
    {
      X->gc_mask = GCForeground | GCBackground ;

/*  This is a server image of depth 1. Therefore, set foreground/background
 *  based on a depth of 1, and shouldn't use BlackPixel/WhitePixel
 *  since these are relative to the default colormaps.
 */

      X->gc_val.foreground = 1 ;
      X->gc_val.background = 0 ;
      X->svgc = XCreateGC(X->dpy, xid, X->gc_mask, &X->gc_val) ;
 
      X->svropgc = XCreateGC(X->dpy, xid, X->gc_mask, &X->gc_val) ;
      XSetFillStyle(X->dpy, X->svropgc, FillOpaqueStippled) ;
    }
 
/*  Create the button. Clear the server image area, draw the appropriate
 *  four corners in, at the appropriate scale. Join each of the corners
 *  together by lines (two extra lines for mono), and fill in the middle
 *  appropriately (nothing for B_NORMAL, a gray patch for color B_INVERT
 *  and solid black for mono B_INVERT and the B_STENCIL images.
 */
 
  XSetFunction(X->dpy, X->svgc, GXclear) ;
  XCopyArea(X->dpy, xid, xid, X->svgc, 0, 0, width, height, 0, 0) ;
  XSetFunction(X->dpy, X->svgc, GXcopy) ;

  add_but_corner(xid, bstate, scale, TL_CORNER, width, height) ;
  add_but_corner(xid, bstate, scale, TR_CORNER, width, height) ;
  add_but_corner(xid, bstate, scale, BL_CORNER, width, height) ;
  add_but_corner(xid, bstate, scale, BR_CORNER, width, height) ;
   
  c = cornerR[(int) scale] ;
  if (bstate == B_INVERT)
    if (v->iscolor && v->is_3D)
      {
        X->gc_mask = GCStipple | GCTileStipXOrigin | GCTileStipYOrigin ;
        X->gc_val.stipple = X->grey_pr ;
        X->gc_val.ts_x_origin = 0 ;
        X->gc_val.ts_y_origin = 0 ;
        XChangeGC(X->dpy, X->svropgc, X->gc_mask, &X->gc_val) ;

        XFillRectangle(X->dpy, xid, X->svropgc,
                       1, c, width - 2, height - (2 * c)) ;
        XFillRectangle(X->dpy, xid, X->svropgc,
                       c, 1, width - (2*c), height - 2) ;
      }
    else
      {
        XFillRectangle(X->dpy, xid, X->svgc,
                       2, c, width - 4, height - (2 * c)) ;
        XFillRectangle(X->dpy, xid, X->svgc,
                       c, 2, width - (2 * c), height - 4) ;
      }
 
  if (bstate == B_STENCIL)
    {
      XFillRectangle(X->dpy, xid, X->svgc, 0, c, width, height - (2 * c)) ;
      XFillRectangle(X->dpy, xid, X->svgc, c, 0, width - (2 * c), height) ;
    }
  else
    { 
      XDrawLine(X->dpy, xid, X->svgc, 0, c, 0, height - c) ;
      XDrawLine(X->dpy, xid, X->svgc, c, height - 1, width - c, height - 1) ;
      XDrawLine(X->dpy, xid, X->svgc, width - 1, c, width - 1, height - c) ;
      XDrawLine(X->dpy, xid, X->svgc, c, 0, width - c, 0) ;

      if ((!v->is_3D || !v->iscolor) && bstate == B_NORMAL)
        {
          XDrawLine(X->dpy, xid, X->svgc, c, height-2, width - c, height-2) ;
          XDrawLine(X->dpy, xid, X->svgc, width-2, c, width-2, height - c) ;
        }
    }
}


void
make_frames()
{
  extern char *ds_relname(), *ds_hostname() ;
  char *tool_label = NULL , *hn;

  if (v->titleline == NULL)
    {
      hn = ds_hostname((Display *) xv_get(Calctool_kframe->kframe, 
			XV_DISPLAY));
      tool_label = malloc(strlen(lstrs[(int) L_UCALC]) + 
                          strlen(ds_relname()) + strlen(hn) + 3);
  
      SPRINTF(tool_label, "%s %s%s", lstrs[(int) L_UCALC], ds_relname(), hn);
    }
  else read_str(&tool_label, v->titleline) ;

  XV_SET(Calctool_kframe->kframe, XV_LABEL,          tool_label,       0) ;
  XV_SET(Calctool_kframe->kframe, FRAME_SHOW_HEADER, v->istitle,       0) ;

  v->iscolor = ((int) xv_get(Calctool_kframe->kframe, XV_DEPTH) > 1) ? 1 : 0 ;

  if (v->monochrome) v->iscolor = 0 ;
  X->sel_client = seln_create(func_key_proc, reply_proc, (char *) 0) ;

  XV_SET(Calctool_rframe->rframe, FRAME_DONE_PROC, done_proc, 0) ;
  XV_SET(Calctool_mframe->mframe, FRAME_DONE_PROC, done_proc, 0) ;
  free(tool_label);
}


void
make_items()
{
  int h, w, x, y ;

  X->menu_pr[(int) B_NORMAL]  = make_server_image(menu_normal_image) ;
  X->menu_pr[(int) B_INVERT]  = make_server_image(menu_invert_image) ;
  X->menu_pr[(int) B_STENCIL] = make_server_image(menu_stencil_image) ;
  X->grey_pr                  = make_server_image(grey_image) ;

  get_frame_size(FCP_KEY, &x, &y, &w, &h) ;
  set_frame_size(FCP_KEY, x, y, v->twidth, v->theight) ;

  get_frame_size(FCP_REG, &x, &y, &w, &h) ;
  set_frame_size(FCP_REG, x, y, v->rwidth, v->rheight) ;

  get_frame_size(FCP_MODE, &x, &y, &w, &h) ;
  set_frame_size(FCP_MODE, x, y, v->mwidth, v->mheight) ;
}


static Drawable
make_server_image(image)
unsigned short image[] ;
{
  Server_image temp ;

  temp = xv_create(XV_NULL,           SERVER_IMAGE,
                   XV_WIDTH,          64,
                   XV_HEIGHT,         64,
                   SERVER_IMAGE_BITS, image,
                   0) ;
  return((int) xv_get(temp, XV_XID)) ;
}


void
make_subframes()
{
  Icon icon ;
  Xv_Font font ;

#ifdef OW_I18N
  if (X->multi_byte)
    {
      X->pf = (Xv_Font) xv_find(Calctool_kframe->kframe, FONT,
                                FONT_FAMILY, FONT_FAMILY_SANS_SERIF,
                                0) ;
      X->fontset = (XFontSet) xv_get(X->pf, FONT_SET_ID) ;
    }
#endif /*OW_I18N*/

  XV_SET(Calctool_kframe->kcanvas,
         CANVAS_AUTO_EXPAND, TRUE,
         CANVAS_AUTO_SHRINK, TRUE,
         OPENWIN_AUTO_CLEAR, FALSE,
         XV_WIDTH,           v->twidth,
         XV_HEIGHT,          v->theight,
         0) ;
 
  XV_SET(Calctool_rframe->rcanvas,
         CANVAS_AUTO_EXPAND, TRUE,
         CANVAS_AUTO_SHRINK, TRUE,
         OPENWIN_AUTO_CLEAR, FALSE,
         XV_WIDTH,           v->rwidth,
         XV_HEIGHT,          v->rheight,
         0) ;
  NOTIFY_INTERPOSE_EVENT_FUNC(Calctool_rframe->rframe,
                              frame_interpose, NOTIFY_SAFE) ;

  XV_SET(Calctool_mframe->mcanvas,
         CANVAS_AUTO_EXPAND, TRUE,
         CANVAS_AUTO_SHRINK, TRUE,
         OPENWIN_AUTO_CLEAR, FALSE,
         XV_WIDTH,           v->mwidth,
         XV_HEIGHT,          v->mheight,
         0) ;

  DS_ADD_HELP(Calctool_rframe->rcanvas,  hstrs[(int) H_MCANVAS]) ;
  DS_ADD_HELP(Calctool_rframe->rframe,  hstrs[(int) H_MFRAME]) ;
  X->pw[(int) FCP_KEY]  = canvas_paint_window(Calctool_kframe->kcanvas) ;
  X->pw[(int) FCP_REG]  = canvas_paint_window(Calctool_rframe->rcanvas) ;
  X->pw[(int) FCP_MODE] = canvas_paint_window(Calctool_mframe->mcanvas) ;

  XV_SET(X->pw[(int) FCP_KEY],  WIN_BIT_GRAVITY, ForgetGravity, 0) ;
  XV_SET(X->pw[(int) FCP_REG],  WIN_BIT_GRAVITY, ForgetGravity, 0) ;
  XV_SET(X->pw[(int) FCP_MODE], WIN_BIT_GRAVITY, ForgetGravity, 0) ;

  X->xid[(int) FCP_KEY]  = (Drawable) xv_get(X->pw[(int) FCP_KEY],  XV_XID) ;
  X->xid[(int) FCP_REG]  = (Drawable) xv_get(X->pw[(int) FCP_REG],  XV_XID) ;
  X->xid[(int) FCP_MODE] = (Drawable) xv_get(X->pw[(int) FCP_MODE], XV_XID) ;

  X->screen = DefaultScreen(X->dpy) ;

/*  Can't use BlackPixel/WhitePixel... get these values from the canvas
 *  instead. (BlackPixel/WhitePixel refer to the DefaultColormap, and
 *  we might have been started with a different depth and/or visual).
 */

  X->foregnd = xv_get(xv_get(Calctool_kframe->kframe, WIN_CMS),
                             CMS_FOREGROUND_PIXEL) ;
  X->backgnd = xv_get(xv_get(Calctool_kframe->kframe, WIN_CMS),
                             CMS_BACKGROUND_PIXEL) ;

  X->gc_mask = GCForeground | GCBackground | GCGraphicsExposures ;
  X->gc_val.foreground = X->foregnd ;
  X->gc_val.background = X->backgnd ;
  X->gc_val.graphics_exposures = False ;

/*  Create GCs using xid of the canvas, not the root (canvas and root
 *  may have different colormaps, visuals...
 */

  X->gc = XCreateGC(X->dpy, X->xid[(int) FCP_KEY], X->gc_mask, &X->gc_val) ;

  X->ropgc     = XCreateGC(X->dpy, X->xid[(int) FCP_KEY],
                           X->gc_mask, &X->gc_val) ;

  X->stencilgc = XCreateGC(X->dpy, X->xid[(int) FCP_KEY],
                           X->gc_mask, &X->gc_val) ;
  XSetFillStyle(X->dpy, X->stencilgc, FillOpaqueStippled) ;

/*  Make a couple of scratch pixmaps for shuffling images to a 0,0 origin
 *  within a pixmap before the image can be manipulated with the screen.
 */

  X->scratch1 = XCreatePixmap(X->dpy, X->xid[(int) FCP_KEY], 32, 32, 1) ;
  X->scratch2 = XCreatePixmap(X->dpy, X->xid[(int) FCP_KEY], 32, 32, 1) ;

  load_colors() ;                       /* Load the calctool colormap. */

  if (X->iconfont)                      /* Set icon font (if specified). */
    {
      icon = (Icon) xv_get(Calctool_kframe->kframe, FRAME_ICON) ;
      font = (Xv_Font) xv_find(Calctool_kframe->kframe, FONT,
                               FONT_NAME, X->iconfont,
                               0) ;
      XV_SET(icon, ICON_FONT, font, 0) ;
    }
}


/*ARGSUSED*/
static void
menu_done_proc(menu, result)
Menu menu ;
Xv_opaque result ;
{
  X->menu_showing = 0 ;
  draw_button(v->curwin, v->row, v->column, B_NORMAL) ;
}


static void
menu_proc(menu, menu_item)
Menu menu ;
Menu_item menu_item ;
{      
  int choice, i ;
  int pinned = 0 ;
  Frame frame ;

  frame = (Frame) xv_get(menu, MENU_PIN_WINDOW) ;
  if (frame) pinned = xv_get(frame, FRAME_CMD_PUSHPIN_IN) ;
 
  X->menu_showing = 0 ;
  choice = (int) xv_get(menu_item, MENU_VALUE) ;
  for (i = 0; i < MAXMENUS; i++)
    if (menu == X->menus[i]) break ;

  if (choice)
    if (menu == X->menus[(int) M_PROPS]) display_prop_sheet() ;
    else handle_menu_selection(X->mrec[i], choice, pinned) ;
}


int
modewin_pinned()
{
  return((int) xv_get(Calctool_mframe->mframe, FRAME_CMD_PUSHPIN_IN) &&
         (int) xv_get(Calctool_mframe->mframe, XV_SHOW)) ;
}


/*ARGSUSED*/
static void
new_cf_value(menu, menu_item)
Menu menu ;
Menu_item menu_item ;
{
  if (Calctool_CFframe == NULL) create_cfframe() ;
  if (menu == X->menus[(int) M_CON])
    {
      X->CFtype = M_CON ;
      XV_SET(Calctool_CFframe->CFpi_cftext,
             PANEL_LABEL_STRING, lstrs[(int) L_CONNO],
             0) ;
      XV_SET(Calctool_CFframe->CFpi_fbut, PANEL_SHOW_ITEM, FALSE, 0) ;
      XV_SET(Calctool_CFframe->CFpi_cbut, PANEL_SHOW_ITEM, TRUE, 0) ;
      XV_SET(Calctool_CFframe->CFframe, FRAME_LABEL, lstrs[(int) L_NEWCON], 0) ;
    }
  else if (menu == X->menus[(int) M_FUN])
    {
      X->CFtype = M_FUN ;
      XV_SET(Calctool_CFframe->CFpi_cftext,
             PANEL_LABEL_STRING, lstrs[(int) L_FUNNO],
             0) ;
      XV_SET(Calctool_CFframe->CFpi_cbut, PANEL_SHOW_ITEM, FALSE, 0) ;
      XV_SET(Calctool_CFframe->CFpi_fbut, PANEL_SHOW_ITEM, TRUE, 0) ;
      XV_SET(Calctool_CFframe->CFframe, FRAME_LABEL, lstrs[(int) L_NEWFUN], 0) ;
    }

/* Clear text fields. */

  XV_SET(Calctool_CFframe->CFpi_cftext, PANEL_VALUE, "", 0) ;
  XV_SET(Calctool_CFframe->CFpi_dtext,  PANEL_VALUE, "", 0) ;
  XV_SET(Calctool_CFframe->CFpi_vtext,  PANEL_VALUE, "", 0) ;

  XV_SET(Calctool_CFframe->CFpanel,
         PANEL_CARET_ITEM, Calctool_CFframe->CFpi_cftext,
         0) ;
  if ((int) xv_get(Calctool_CFframe->CFframe, XV_SHOW) == FALSE)
    ds_position_popup(Calctool_kframe->kframe,
                      Calctool_CFframe->CFframe, DS_POPUP_RIGHT) ;
  XV_SET(Calctool_CFframe->CFframe, XV_SHOW, TRUE, 0) ;
}


/*ARGSUSED*/
void
prop_apply(item, event)
Panel_item item ;
Event *event ;
{
  int oldm, oldr, oldt ;

  oldm          = v->monochrome ;
  oldr          = v->righthand ;
  oldt          = v->is_3D ;
  v->is_3D      = (int) xv_get(Calctool_Pframe->Pappearance, PANEL_VALUE) ;
  v->monochrome = (int) xv_get(Calctool_Pframe->Pdisplay,    PANEL_VALUE) ;
  v->righthand  = (int) xv_get(Calctool_Pframe->Pstyle,      PANEL_VALUE) ;
  set_prop_options(oldm, oldr, oldt) ;
  write_cmdline() ;
}


/*ARGSUSED*/
void
prop_defaults(item, event)
Panel_item item ;
Event *event ;
{
  prop_apply(item, event) ;
  write_resources() ;
}


/*ARGSUSED*/
void
prop_reset(item, event)
Panel_item item ;
Event *event ;
{
  int oldm, oldr, oldt ;
 
  oldm = v->monochrome ;
  oldr = v->righthand ;
  oldt = v->is_3D ;
  reset_prop_vals() ;
  set_prop_options(oldm, oldr, oldt) ;
  write_cmdline() ;
}


void
put_resource(rtype, value)   /* Put clock resource into deskset database. */
enum res_type rtype ;
char *value ;
{
  ds_put_resource(&X->desksetDB, v->appname, calc_res[(int) rtype], value) ;
}


/*ARGSUSED*/
static Seln_result
reply_proc(item, context, length)
Seln_attribute item ;
Seln_replier_data *context ;
int length ;
{
  int size ;
  char *destp ;

  switch (item)
    {
      case SELN_REQ_CONTENTS_ASCII :

             if (context->context == NULL)
               {
                 if (v->shelf == NULL) return(SELN_DIDNT_HAVE) ;
                 context->context = v->shelf ;
               }
             size  = strlen(context->context) ;
             destp = (char *) context->response_pointer ;
             STRCPY(destp, context->context) ;
             destp += size ;
             while ((int) destp % 4 != 0) *destp++ = '\0' ;
             context->response_pointer = (char **) destp ;
             *context->response_pointer++ = 0 ;
             return(SELN_SUCCESS) ;

      case SELN_REQ_YIELD :

             *context->response_pointer++ = (char *) SELN_SUCCESS ;
             return(SELN_SUCCESS) ;

      case SELN_REQ_BYTESIZE :

             if (v->shelf == NULL) return(SELN_DIDNT_HAVE) ;
             *context->response_pointer++ = (char *) strlen(v->shelf) ;
             return(SELN_SUCCESS) ;

      case SELN_REQ_END_REQUEST : return(SELN_SUCCESS) ;

      default                   : return(SELN_UNRECOGNIZED) ;
    }
}


void
reset_prop_vals()
{
  XV_SET(Calctool_Pframe->Pappearance, PANEL_VALUE, v->is_3D,         0) ;
  XV_SET(Calctool_Pframe->Pdisplay,    PANEL_VALUE, v->monochrome,    0) ;
  XV_SET(Calctool_Pframe->Pstyle,      PANEL_VALUE, v->righthand,     0) ;
}


void
save_cmdline(argc, argv)
int argc ;
char *argv[] ;
{
  DS_SAVE_CMDLINE(Calctool_kframe->kframe, argc, argv) ;
}


void
save_resources()
{
  ds_save_resources(X->desksetDB) ;
}


static void
set_color(value, isforegnd, isreversed)
char *value ;
int isforegnd, isreversed ;
{
  char colstr[MAXLINE] ;
  int red, green, blue ;

  if (value != NULL && *value != '\0')
    {
      if (isreversed) isforegnd = !isforegnd ;
      if (sscanf(value, "%d %d %d", &red, &green, &blue) == 3)
        {
          SPRINTF(colstr, "#%02x%02x%02x", red, green, blue) ;
          value = colstr ;
        }
      if (isforegnd)
        {
          if (v->colstr[C_TEXT] == NULL)
            read_str(&v->colstr[C_TEXT], value) ;
        }
      else
        {
          if (v->colstr[C_DISPCOL] == NULL)
            read_str(&v->colstr[C_DISPCOL], value) ;
          if (v->colstr[C_MEMORY] == NULL)
            read_str(&v->colstr[C_MEMORY], value) ;
        }
    }
}


void
set_frame_size(fcptype, x, y, w, h)
enum fcp_type fcptype ;
int x, y, w, h ;
{
  Canvas c ;
  Frame f ;

       if (fcptype == FCP_KEY)  f = Calctool_kframe->kframe ;
  else if (fcptype == FCP_REG)  f = Calctool_rframe->rframe ;
  else if (fcptype == FCP_MODE) f = Calctool_mframe->mframe ;
  DS_SET_FRAME_SIZE(f, x, y, w, h) ;

  if (fcptype != FCP_KEY)
    {
           if (fcptype == FCP_REG)  c = Calctool_rframe->rcanvas ;
      else if (fcptype == FCP_MODE) c = Calctool_mframe->mcanvas ;
      XV_SET(c,
             XV_WIDTH,  w,
             XV_HEIGHT, h,
             0) ;
    }
}


/*  We can't allocate the full calctool colormap, so instead we cook up a
 *  special colormap using black, white, the workspace color, the default
 *  window color, a shade of gray (for the 3D button inversion), plus the
 *  tools foreground and background colors. The only new color should be
 *  the shade of grey. If we can't get that, or if this all fails for some
 *  other reason, then return FALSE, forcing calctool to monochrome.
 *  Color palette indices will be:
 *
 *  C_WHITE   = white pixel color.
 *  C_BLACK   = black pixel color.
 *  C_GREY    = grey color.
 *  C_BACK    = workspace color.
 *  C_DISPCOL = tool background color.
 *  C_MEMORY  = tool background color.
 *  C_TEXT    = tool foreground color.
 *  the rest will be the window color.
 */

int
set_min_colors()
{
  Cms cms ;
  Colormap def_cmap ;
  char *str_type[20] ;
  XColor ccol ;
  XrmValue value ;
  int i ;

  cms      = (Cms)      xv_get(Calctool_kframe->kframe, WIN_CMS) ;
  def_cmap = (Colormap) xv_get(cms, CMS_CMAP_ID) ;

/* Setup window color initially in every palette entry. */

  if (XrmGetResource(X->rDB, WINDOW_RES, WINDOW_CLASS_RES,
                     str_type, &value) == NULL)
    return(FALSE) ;

  if (XParseColor(X->dpy, def_cmap, value.addr, &ccol) == 0) return(FALSE) ;
  if (XAllocColor(X->dpy, def_cmap, &ccol) != True)          return(FALSE) ;

  for (i = 0; i < CALC_COLORSIZE; i++) X->palette[i] = ccol.pixel ;

/* Setup workspace color in C_BACK palette entry. */

  if (XrmGetResource(X->rDB, WORKSPACE_RES, WORKSPACE_CLASS_RES,
                     str_type, &value) == NULL)
    return(FALSE) ;
  
  if (XParseColor(X->dpy, def_cmap, value.addr, &ccol) == 0) return(FALSE) ;
  if (XAllocColor(X->dpy, def_cmap, &ccol) != True)          return(FALSE) ;
  
  X->palette[C_BACK] = ccol.pixel ;

/* Setup shade of grey. */

  ccol.flags = DoRed | DoGreen | DoBlue ;
  ccol.red   = (unsigned short) (v->rcols[C_GREY] << 8) ;
  ccol.green = (unsigned short) (v->gcols[C_GREY] << 8) ;
  ccol.blue  = (unsigned short) (v->bcols[C_GREY] << 8) ;
  if (XAllocColor(X->dpy, def_cmap, &ccol) != True) return(FALSE) ;
  X->palette[C_GREY] = ccol.pixel ;

/* Setup C_WHITE, C_BLACK, C_DISPCOL, C_MEMORY and C_TEXT. */

/*  Can't use BlackPixel/WhitePixel... get these values from the canvas
 *  instead. (BlackPixel/WhitePixel refer to the DefaultColormap, and
 *  we might have been started with a different depth and/or visual).
 */

  X->palette[C_BLACK]   = xv_get(xv_get(Calctool_kframe->kframe, WIN_CMS),
                                 CMS_FOREGROUND_PIXEL) ;
  X->palette[C_WHITE]   = xv_get(xv_get(Calctool_kframe->kframe, WIN_CMS),
                                 CMS_BACKGROUND_PIXEL) ;

  X->palette[C_DISPCOL] = xv_get(cms, CMS_BACKGROUND_PIXEL) ;
  X->palette[C_MEMORY]  = X->palette[C_DISPCOL] ;
  X->palette[C_TEXT]    = xv_get(cms, CMS_FOREGROUND_PIXEL) ;
  return(TRUE) ;
}


void
set_prop_options(oldm, oldr, oldt)
int oldm, oldr, oldt ;
{
  enum base_type curbase ;

  if (v->monochrome && v->is_3D)
    {
      v->is_3D = 0 ;
      XV_SET(Calctool_Pframe->Pappearance, PANEL_VALUE, v->is_3D, 0) ;
    }

  if (oldm != v->monochrome || oldt != v->is_3D)
    {
      if (v->monochrome) v->iscolor = 0 ;
      else v->iscolor = ((int) xv_get(Calctool_kframe->kframe,
                                      XV_DEPTH) > 1) ? 1 : 0 ;
      if (v->iscolor && !X->cmap_loaded) load_colors() ;
      load_corners() ;
      make_buttons() ;
    }

  if (oldr != v->righthand)
    {
      curbase = v->base ;
      grey_buttons(HEX) ;
      switch_hands(v->righthand) ;
      grey_buttons(curbase) ;
    }

  v->curwin = FCP_KEY ;
  make_canvas(0, 0, v->twidth, v->theight, 0) ;
  make_registers() ;

  if (v->modetype != BASIC)
    {
      v->curwin = FCP_MODE ;
      make_modewin(0, 0, v->mwidth, v->mheight) ;
    }
}


void
set_title(fcptype, str)     /* Set new title for a window. */
enum fcp_type fcptype ;
char *str ;
{
  Frame f ;

       if (fcptype == FCP_KEY)  f = Calctool_kframe->kframe ;
  else if (fcptype == FCP_REG)  f = Calctool_rframe->rframe ;
  else if (fcptype == FCP_MODE) f = Calctool_mframe->mframe ;
  XV_SET(f, FRAME_LABEL, str, 0) ;
}


void
show_ascii_frame()      /* Display ASCII popup. */
{
  if (Calctool_Aframe == NULL) create_aframe() ;
  if ((int) xv_get(Calctool_Aframe->Aframe, XV_SHOW) == FALSE)
    ds_position_popup(Calctool_kframe->kframe,
                      Calctool_Aframe->Aframe, DS_POPUP_LEFT) ;
  XV_SET(Calctool_Aframe->Aframe, XV_SHOW, TRUE, 0) ;
}


void
show_help(str)     /* Display help using this label. */
char *str ;
{
  xv_help_show(canvas_paint_window (Calctool_kframe->kcanvas), str,
	X->cur_event) ;
}


void
start_popup(mtype)    /* Start constant/function popup. */
enum menu_type mtype ;
{
  new_cf_value(X->menus[(int) mtype],
               (Menu_item) xv_get(X->menus[(int) mtype], MENU_NTH_ITEM, 1)) ;
}


void
start_tool()
{
  v->started = 1 ;
  xv_main_loop(Calctool_kframe->kframe) ;
}


Panel_setting
tshow_ascii(item, event)
Panel_item item ;
Event *event ;
{
  (void) bshow_ascii(item, event) ;
  return(panel_text_notify(item, event)) ;
}


void
win_display(fcptype, state)
enum fcp_type fcptype ;
int state ;
{
  Frame f ;

       if (fcptype == FCP_REG)  f = Calctool_rframe->rframe ;
  else if (fcptype == FCP_MODE) f = Calctool_mframe->mframe ;

  if (state && xv_get(f, XV_SHOW))
    {
      wmgr_top(f) ;
      return ;
    }
  if (state)
    {
           if (fcptype == FCP_REG)
        ds_position_popup(Calctool_kframe->kframe, f, DS_POPUP_ABOVE) ;
      else if (fcptype == FCP_MODE)
        ds_position_popup(Calctool_kframe->kframe, f, DS_POPUP_BELOW) ;
    }
  XV_SET(f, FRAME_CMD_PUSHPIN_IN, state, 0) ;
  XV_SET(f, XV_SHOW, state, 0) ;
}


/*ARGSUSED*/
void
write_cf_value(item, event)
Panel_item item ;
Event *event ;
{
  Frame frame ;
  Menu_item mi ;
  char str[MAXLINE] ;      /* Temporary buffer for various strings. */
  char *pval ;             /* Points to values returned from panel items. */
  int cfno ;               /* Current constant/function number. */
  int exists ;             /* Set if the constant/function exists. */
  int i ;
  int n ;                  /* Set to 1, if constant value is valid. */
  int pinned = FALSE ;     /* Set if this menu is pinned. */
  int result ;
  double tmp ;             /* For converting constant value. */

  pval = (char *) xv_get(Calctool_CFframe->CFpi_cftext, PANEL_VALUE) ;
  SSCANF(pval, "%d", &cfno) ;
  if (cfno < 0 || cfno > 9)
    {
      SPRINTF(str, vstrs[(int) V_INVALID],
              (X->CFtype == M_CON) ? vstrs[(int) V_LCON]
                                   : vstrs[(int) V_LFUN]) ;
      notice_prompt(Calctool_CFframe->CFframe, event,
                    NOTICE_MESSAGE_STRINGS,
                      str,
                      vstrs[(int) V_RANGE], 0,
                    NOTICE_BUTTON, vstrs[(int) V_CONTINUE], 0,
                    0) ;
      return ;
    }
 
  exists = 0 ;
  switch (X->CFtype)
    {
      case M_CON : exists = 1 ;    /* Always the default constants. */
                   break ;
      case M_FUN : if (strlen(v->fun_vals[cfno])) exists = 1 ;
    }
  if (exists)
    {
      SPRINTF(str, mess[(int) MESS_CON],
                   (X->CFtype == M_CON) ? vstrs[(int) V_UCON]
                                        : vstrs[(int) V_UFUN], cfno) ;
      result = notice_prompt(Calctool_CFframe->CFframe, (Event *) NULL,
                             NOTICE_MESSAGE_STRINGS,
                               str,
                               vstrs[(int) V_OWRITE], 0,
                             NOTICE_BUTTON_YES, vstrs[(int) V_CONFIRM],
                             NOTICE_BUTTON_NO,  vstrs[(int) V_CANCEL],
                             0) ;
      switch (result)
        {
          case NOTICE_YES    : break ;
          case NOTICE_NO     :
          case NOTICE_FAILED : return ;
        }
    }    

  frame = (Frame) xv_get(X->menus[(int) X->CFtype], MENU_PIN_WINDOW) ;
  if (frame) pinned = xv_get(frame, FRAME_CMD_PUSHPIN_IN) ;
  if (pinned) XV_SET(frame, FRAME_CMD_PUSHPIN_IN, FALSE, 0) ;

  pval = (char *) panel_get_value(Calctool_CFframe->CFpi_vtext) ;
  switch (X->CFtype)
    {
      case M_CON : n = sscanf(pval, "%lf", &tmp) ;
                   if (n != 1)
                     {                 
                       notice_prompt(Calctool_CFframe->CFframe, event,
                                     NOTICE_MESSAGE_STRINGS,
                                       vstrs[(int) V_INVCON],
                                       vstrs[(int) V_NOCHANGE], 0,
                                     NOTICE_BUTTON, vstrs[(int) V_CONTINUE], 0,
                                     0) ;
                       return ;
                     } 
                   MPstr_to_num(pval, DEC, v->MPcon_vals[cfno]) ;
                   SPRINTF(v->con_names[cfno], "%1d: %s [%s]",
                           cfno, pval,
                           xv_get(Calctool_CFframe->CFpi_dtext, PANEL_VALUE)) ;
                   break ;
      case M_FUN : STRCPY(v->fun_vals[cfno],
                          convert((char *) xv_get(Calctool_CFframe->CFpi_vtext,
                                                  PANEL_VALUE))) ;
                   SPRINTF(v->fun_names[cfno], "%1d: %s [%s]",
                           cfno,
                           xv_get(Calctool_CFframe->CFpi_vtext, PANEL_VALUE),
                           xv_get(Calctool_CFframe->CFpi_dtext, PANEL_VALUE)) ;
    }

  pval = (X->CFtype == M_CON) ? v->con_names[cfno] : v->fun_vals[cfno] ;
  if (X->CFtype == M_CON && exists)
    {
      mi = (Menu_item) xv_get(X->menus[(int) X->CFtype],
                              MENU_NTH_ITEM, cfno+3) ;
      XV_SET(mi, MENU_STRING, pval, 0) ;
    }
  else
    {

/*  All the items in the constant/function menu (apart from the first two
 *  which includes the pin/title item) are destroyed, and the menu is built
 *  up again from the saved values.
 */
 
      for (i = (int) xv_get(X->menus[(int) X->CFtype], MENU_NITEMS); i > 2; i--)
        {
          mi = xv_get(X->menus[(int) X->CFtype], MENU_NTH_ITEM, i) ;
          XV_SET(X->menus[(int) X->CFtype], MENU_REMOVE, i, 0) ;
          XV_DESTROY_SAFE(mi) ;
        }
      for (i = 0; i < NOBUTTONS; i++)
        if (buttons[i].mtype == X->CFtype) create_con_fun_menu(X->CFtype, i) ;
    }
  if (pinned) XV_SET(frame, FRAME_CMD_PUSHPIN_IN, TRUE, 0) ;

  write_rcfile(X->CFtype, exists, cfno,
              (char *) xv_get(Calctool_CFframe->CFpi_vtext, PANEL_VALUE),
              (char *) xv_get(Calctool_CFframe->CFpi_dtext, PANEL_VALUE)) ;
}


/*ARGSUSED*/
static int
xview_error_proc(object, avlist)
Xv_object object ;
Attr_avlist avlist ;
{
  Attr_avlist attrs ;
  Error_severity severity = ERROR_RECOVERABLE ;

  if (v->MPdebug)
    FPRINTF(stderr, "\nXView Error (Intercepted)\n") ;

  for (attrs = avlist; *attrs ; attrs = attr_next(attrs))
    {
      switch((int) attrs[0])
        {
          case ERROR_BAD_ATTR :
            if (v->MPdebug)
              FPRINTF(stderr, "Bad Attribute:%s\n", attr_name(attrs[1])) ;
            break ;

          case ERROR_BAD_VALUE :
            if (v->MPdebug)
              FPRINTF(stderr, "Bad Value (0x%x) for attribute: %s\n",
                              attrs[1], attr_name(attrs[2])) ;
            break ;

          case ERROR_INVALID_OBJECT :
            if (v->MPdebug)
              FPRINTF(stderr, "Invalid Object: %s\n", (char *) attrs[1]) ;
            break ;

          case ERROR_STRING :
            {
              char *c = (char *) attrs[1] ;

              if (c[strlen(c)] == '\n')
                c[strlen(c)] = '\0' ;
              FPRINTF(stderr, "%s\n", (char *) attrs[1]) ;
            }
          break ;

          case ERROR_PKG :
            if (v->MPdebug)
              FPRINTF(stderr, "Package: %s\n", ((Xv_pkg *) attrs[1])->name) ;
            break ;

          case ERROR_SEVERITY : severity = attrs[1] ;
                                break ;

          default :
            if (v->MPdebug)
              FPRINTF(stderr, "Unknown XView error Attribute (%s)\n",
                      (char *) attrs[1]) ;
        }
    }    

/* If a critical error or debugging, then core dump. */

  if (severity == ERROR_NON_RECOVERABLE || v->MPdebug)
    {
      char *cwd ;    /* Current working directory. */

      if (!v->MPdebug) exit(1) ;
      if (severity == ERROR_NON_RECOVERABLE)
        FPRINTF(stderr, "Non-recoverable Error: \ndumping core file") ;
      else
        FPRINTF(stderr, "Debug mode: Recoverable Error: \ndumping core file") ;

      cwd = (char *) getcwd(NULL, MAXPATHLEN) ;
      if (cwd) FPRINTF(stderr, " in %s\n", cwd ) ;
      else     FPRINTF(stderr, "...\n") ;

      abort() ;
    }
  else
    { 
      if (v->MPdebug)
        FPRINTF(stderr, "Recoverable Error: continuing...\n") ;
      return(XV_OK) ;
    }
/*NOTREACHED*/
}
