#ifndef lint
static  char sccsid[] = "@(#)xview.c 1.25 94/08/23 Copyright (c) Sun Microsystems Inc." ;
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

#include <unistd.h>
#include "perfmeter.h"
#include <euc.h>
#include <xview/xview.h>
#include <xview/notice.h>
#include <xview/font.h>
#include <xview/panel.h>
#include <xview/svrimage.h>
#include <xview/xv_xrect.h>
#include <xview/cms.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include "ds_xlib.h"
#include "ds_popup.h"

#include "perfmeter_ui.h"

static unsigned short ricon_data[] = {    /* Icons for the meters. */
#include "images/rspeedometer.icon"
} ;

static unsigned short dead_data[] = {
#include "images/dead.icon"
} ;

static unsigned short sick_data[] = {
#include "images/sick.icon"
} ;

#define  DS_ADD_HELP                  (void) ds_add_help
#define  DS_SAVE_CMDLINE              (void) ds_save_cmdline
#define  METER_ITIMER_EXPIRED         (void) meter_itimer_expired
#define  NOTIFY_INTERPOSE_DESTROY_FUNC  (void) notify_interpose_destroy_func
#define  NOTIFY_INTERPOSE_EVENT_FUNC  (void) notify_interpose_event_func
#define  NOTIFY_SET_ITIMER_FUNC       (void) notify_set_itimer_func
#define  XV_CREATE                    (void) xv_create
#define  XV_SET                       (void) xv_set

#define  DEFFONT     "fixed"      /* Default font name. */
#define  MENU_PROPS  MAXMETERS+2  /* Index to Properties... menu entry. */

extern char *hstr[] ;             /* Spot help messages. */
extern char *mess[] ;             /* Message strings. */
extern char *nstrs[] ;            /* Notice strings. */
extern char *perf_res[] ;         /* Perfmeter resource strings. */
extern char *pmenu_items[] ;      /* Property menu string values. */
extern char *pmenu_help[] ;       /* Frame menu help values. */
extern char *vstrs[] ;            /* Various strings. */
extern Data d ;                   /* Perfmeter static data. */
extern Vars v ;                   /* Perfmeter variables and options. */

extern Notify_value meter_itimer_expired P((Notify_client, int)) ;

typedef struct Xobject {          /* XView/Xlib graphics object. */
  Canvas_paint_window pw ;
  Cms cms ;
  Icon meter_icon ;
  Menu pm_menu ;
  Panel_item cur_item ;
  Server_image sv_icon_image, sv_icon_mask_image ;

#ifdef OW_I18N
  XFontSet font_set ;
  Xv_Font pf ;
#endif /*OW_I18N*/

  Display *dpy ;              /* Display ids of perfmeter frames. */
  Drawable images[3] ;        /* Server images blitted onto the display. */
  Drawable wid ;              /* Xid for perfmeter open window. */
  GC gc ;                     /* Main drawing graphics context. */
  GC ropgc ;                  /* Graphics context for rops. */
  Window root ;
  XFontStruct *font ;         /* Xlib handle to the font. */
  XGCValues gc_val ;          /* Used to setup graphics context values. */
  XrmDatabase desksetDB ;     /* Deskset resources database. */
  XrmDatabase rDB ;           /* Combined resources database. */
  char *home ;                /* Pointer to user's home directory. */
  int ch_ascent ;             /* Character ascent of current font. */
  int firstpaint ;            /* TRUE for first valid canvas repaint. */
  int meter_client ;
  int multi_byte ;            /* Set TRUE if multi-byte locale. */
  int opvals[2] ;             /* Rasterop values used. */
  int props_item_key ;
  int repaint ;               /* Set in canvas_repaint when ready. */
  int screen ;                /* Default graphics display screen. */
  int started ;               /* Kludge to prevent excessive repainting. */
  unsigned long backgnd ;     /* Default background color. */
  unsigned long foregnd ;     /* Default foreground color. */
  unsigned long gc_mask ;     /* Mask for graphics context values. */

  unsigned long palette[MAXMETERS+1] ;   /* Xlib color palette. */
} XObject ;

typedef struct Xobject *XVars ;

Notify_value frame_interpose       P((Frame, Event *,
                                      Notify_arg, Notify_event_type)) ;
Notify_value meter_event           P((Xv_Window, Event *,
                                      Notify_arg, Notify_event_type)) ;
Notify_value pm_props_apply_proc   P((Panel_item, Event *)) ;
Notify_value pm_props_default_proc P((Panel_item, Event *)) ;
Notify_value pm_props_reset_proc   P((Panel_item, Event *)) ;

void canvas_repaint                P((Canvas, Xv_Window, Display *,
                                      Xv_Window, Xv_xrectlist *)) ;
void canvas_resize                 P((Canvas, int, int)) ;
void pm_display_choice_proc        P((Panel_item, int, Event *)) ;
void pm_log_choice_proc            P((Panel_item, int, Event *)) ;
void pm_machine_choice_proc        P((Panel_item, int, Event *)) ;

static Notify_value destroy_tool   P((Notify_client, Destroy_status)) ;
static XRectangle compress_repaint P((Xv_Window, Xv_xrectlist *)) ;
static Xv_opaque  do_menu          P((Menu, Menu_item)) ;

static void load_colors            P(()) ;
static void xerror_interpose       P((Display *, XErrorEvent *)) ;

XVars X ;
Xv_notice notice = NULL;

Attr_attribute  INSTANCE ;
perfmeter_frame_objects          *Perfmeter_frame ;
perfmeter_pm_props_frame_objects *Perfmeter_pm_props_frame ;


int
main(argc, argv)
int argc ;
char **argv ;
{
  char bind_home[MAXPATHLEN] ;

  X = (XVars) LINT_CAST(calloc(1, sizeof(XObject))) ;
  INSTANCE      = xv_unique_key() ;
  X->firstpaint = TRUE ;
  X->home       = getenv("HOME") ;

  ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home) ;
  bindtextdomain(MSGFILE_ERROR,   bind_home) ;
  bindtextdomain(MSGFILE_LABEL,   bind_home) ;
  bindtextdomain(MSGFILE_MESSAGE, bind_home) ;

  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
          XV_USE_LOCALE,         TRUE,
          XV_X_ERROR_PROC,       xerror_interpose,
          0) ;

  Perfmeter_frame = perfmeter_frame_objects_initialize(NULL, NULL) ;
  Perfmeter_pm_props_frame = NULL ;
  if (multibyte) X->multi_byte = TRUE ;

  XV_SET(Perfmeter_frame->frame,
         FRAME_NO_CONFIRM,  TRUE,
#ifdef OW_I18N
/*         WIN_USE_IM, FALSE,     XXXXXXX temporary   */
#endif /*OW_I18N*/
         XV_HELP_DATA,      hstr[(int) H_MFRAME],
         0) ;
  X->dpy = (Display *) xv_get(Perfmeter_frame->frame, XV_DISPLAY) ;

  d = (Data) LINT_CAST(calloc(1, sizeof(PerfStaticData))) ;
  v = (Vars) LINT_CAST(calloc(1, sizeof(PerfVars))) ;
  do_perfmeter(argc, argv) ;
  exit(0) ;
/*NOTREACHED*/
}


void
adjust_frame_size()      /* Set perfmeter open frame dimensions. */
{

/*  To remain compatible with OW V2, if there was no initial size given,
 *  and there is only one graph being displayed, and we are monitoring a
 *  remote machine, then the height should be the same as if we were
 *  monitoring a local machine.
 *
 *  Similarily, if we are displaying horizontally.
 */

  switch (v->direction)
    {
      case HORIZONTAL : if (v->iwidth == -1)
                          v->iwidth = v->iconwidth * v->dlen ;
                        if (v->iheight == -1) v->iheight = v->iconheight ;
                        break ;
      case VERTICAL   : if (v->iwidth == -1) v->iwidth = v->iconwidth ;
                        if (v->iheight == -1)
                          {
                            v->iheight = v->iconheight * (v->dlen - 1) ;
                            if (v->remote && v->dlen > 1)
                              v->iheight += v->riconheight ;
                            else v->iheight += v->iconheight ;
                          }
    }

  if (v->dtype == DIAL) set_dial_dims(&v->iwidth, &v->iheight, v->dlen) ;
  XV_SET(Perfmeter_frame->frame,
         FRAME_SHOW_HEADER, v->istitle,
         XV_WIDTH,          v->iwidth,
         XV_HEIGHT,         v->iheight,
         0) ;
  set_dims(v->iwidth, v->iheight) ;
}


/*ARGSUSED*/
void
canvas_repaint(canvas, pw, display, xid, xrects)
Canvas canvas ;             /* Canvas handle */
Xv_Window pw ;              /* Pixwin/Paint window */
Display *display ;          /* Server connection */
Xv_Window xid ;             /* X11 window handle */
Xv_xrectlist *xrects ;      /* Damage rectlist */
{
  static int wwidth = 0 ;
  int w ;
  XRectangle xr ;

/* If window width changes; repaint entire window. */

  if (!X->started) return ;
  X->repaint = TRUE ;        /* Ready to handle size hints now. */

/*  The window manager does not read the NormalHints property until the
 *  frame comes out of it's withdrawn state. So we set the initial minimal
 *  size hints here, the very first time through.
 */

  if (X->firstpaint == TRUE)
    { 
      X->firstpaint = FALSE ;
      set_frame_hints() ;
    }

  w = (int) xv_get(canvas, WIN_WIDTH) ;
  if (w != wwidth)
    {
      xrects = NULL ;
      wwidth = w ;
    }

  if (xrects)
    {
      xr = compress_repaint(xid, xrects) ;
      repaint_from_cache(xr.x, xr.y, xr.width, xr.height) ;
    }
  else meter_paint() ;
}


/*ARGSUSED*/
void
set_size()
{
  Rect *r ;

  r = (Rect *) xv_get(Perfmeter_frame->frame, FRAME_OPEN_RECT) ;
  v->winwidth  = r->r_width ;
  v->winheight = r->r_height ;
}

/*ARGSUSED*/
void
canvas_resize(canvas, width, height)
Canvas canvas ;
int width, height ;
{
  Rect *r ;

  if (!X->started) return ;

  adjust_cache_size(width, height) ;
  r = (Rect *) xv_get(Perfmeter_frame->frame, FRAME_OPEN_RECT) ;
  v->winwidth  = r->r_width ;
  v->winheight = r->r_height ;
  if (v->dtype == GRAPH) set_dims(r->r_width, r->r_height) ;
  meter_paint() ;
}


/*  Checks for font, width, height, scale, header, geometry, icon image
 *  and icon label. 
 */

void
check_args()     /* Check for font, size, label, scale and position. */
{
  char *tempstr ;

  if (defaults_exists("font.name", "Font.Name"))
    {
      tempstr = (char *) defaults_get_string("font.name",
                                             "Font.Name", DEFFONT) ;
      read_str(&v->fontname, tempstr) ;
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
 
  if (defaults_exists("window.scale", "Window.Scale"))
    {
      tempstr = (char *) defaults_get_string("window.scale",
                                             "Window.Scale", "medium") ;
      get_font_scale(tempstr) ;
    }
  if (defaults_exists("window.header", "Window.Header"))
    {
      tempstr = (char *) defaults_get_string("window.header",
                                             "Window.Header", "perfmeter") ;
      if (tempstr == NULL || *tempstr == '\0') tempstr = " " ;
      read_str(&v->titleline, tempstr) ;
      v->istitle = TRUE ;
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
          FPRINTF(stderr, mess[(int) M_ICON], tempstr) ;
          xv_usage(vstrs[(int) V_ILABEL]) ;
          exit(1) ;
        }
    }
  if (defaults_exists("icon.footer", "Icon.Footer"))
    {
      tempstr = (char *) defaults_get_string("icon.footer", "Icon.Footer",
                                             vstrs[(int) V_ILABEL]) ;
      if (tempstr && *tempstr == '\0') tempstr = NULL ;
      read_str(&v->iconlabel, tempstr) ;
    }
}


void
clear_area(x, y, width, height)
int x, y, width, height ;
{            
  if (!X->started) return ;
  X->gc_val.foreground = X->backgnd ;
  X->gc_val.function = GXcopy ;
  XChangeGC(X->dpy, X->gc, GCForeground | GCFunction, &X->gc_val) ;
  XFillRectangle(X->dpy, X->wid, X->gc, x, y, width, height) ;
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


static Notify_value
destroy_tool(client, status)
Notify_client client ;
Destroy_status status ;
{
  if (status == DESTROY_SAVE_YOURSELF) write_cmdline() ;
  if (status == DESTROY_CLEANUP) 
    return notify_next_destroy_func(client, status) ;
  return(NOTIFY_DONE) ;
}


notice_cleared(notice, value, event)
Xv_Notice notice ;
int value ;
Event *event ;
{
  v->remote = FALSE ;
  item_set_val(IMACH, 0) ;     /* Reset to local machine. */
  v->hostname = NULL ;
  set_frame_hints() ;
  resize_display(v->dtype, v->dlen, v->dlen, v->direction, v->direction) ;
  set_timer(v->sampletime) ;
  meter_paint() ;
}


void
display_prompt(etype)
enum error_type etype ;
{
  Event ie ;

  if (!notice)
    notice = (Xv_notice ) xv_create(Perfmeter_frame->frame, NOTICE, 
                                    XV_SHOW, FALSE,
                                    NULL);

  switch ((int) etype)
    {
      case ENOTFOUND : 
		       XV_SET(notice,
                                     NOTICE_MESSAGE_STRINGS,
                                       nstrs[(int) N_NOTFOUND],
                                       0,
                                     NOTICE_BUTTON_YES, nstrs[(int) N_CONT],
                                     NOTICE_EVENT_PROC, notice_cleared,
                                     XV_SHOW, TRUE,
                                     0) ;

                       if (Perfmeter_pm_props_frame != NULL)
                         XV_SET(Perfmeter_pm_props_frame->machine_name,
                                PANEL_INACTIVE, TRUE,
                                0) ;
                       break ;
      case EBADLOG  : 
		       XV_SET(notice,
                                    NOTICE_MESSAGE_STRINGS,
                                      nstrs[(int) N_BADLOG],
                                      0,
                                    NOTICE_BUTTON_YES, nstrs[(int) N_CONT],
                                    XV_SHOW, TRUE,
                                    0) ;
                      if (Perfmeter_pm_props_frame != NULL)
                        XV_SET(Perfmeter_pm_props_frame->log_name,
                               PANEL_INACTIVE, TRUE,
                               0) ;
    }

  if (v->curch != CHAR_MONTYPE && X->cur_item)
    XV_SET(X->cur_item, PANEL_NOTIFY_STATUS, XV_ERROR, 0) ;
}


static Xv_opaque
do_menu(menu, item)
Menu menu ;
Menu_item item ;
{
  int i, n ;
  int itrackval = 0 ;
  int olddlen   = v->dlen ;

  n = (int) xv_get(item, MENU_CLIENT_DATA) ;
  if (n != MENU_PROPS)
    {

/*  If there is one one dial/chart currently being displayed, then this new
 *  selection will replace it. If there is more than one, then the new entry
 *  is appended to the list (assuming it's not already there).
 */

      if (v->dlen == 1)
        {
          v->meters[v->visible].m_showing = 0 ;
          v->meters[n].m_showing = 1 ;
        }
      else 
	{
	  if (!v->meters[n].m_showing)
	    v->meters[n].m_showing = v->dlen + 1 ;
        }

      v->visible = n ;
      v->dlen = 0 ;
      for (i = MAXMETERS-1; i >= 0; i--)
        {
          itrackval = (itrackval << 1) + (v->meters[i].m_showing != 0) ;
          if (v->meters[i].m_showing) v->dlen++ ;
        }
      if (olddlen != v->dlen)
        resize_display(v->dtype, olddlen, v->dlen, v->direction, v->direction) ;
      if (v->isprops == TRUE) xv_item_set_val(ITRACK, itrackval) ;
      set_frame_hints() ;
    }    
  else pm_create_props() ;
 
  meter_paint() ;
  return(item) ;
}


void         
draw_line(x1, y1, x2, y2, color)
int x1, y1, x2, y2;
enum col_type color ;
{            
  if (!X->started) return ;
  if (v->iscolor == FALSE || color == C_FOREGND)
    X->gc_val.foreground = X->foregnd ;
  else X->gc_val.foreground = X->palette[(int) color] ;
 
  X->gc_val.function = GXcopy ;
  XChangeGC(X->dpy, X->gc, GCForeground | GCFunction, &X->gc_val) ;
  XDrawLine(X->dpy, X->wid, X->gc, x1, y1, x2, y2) ;
}


void         
draw_image(x, y, width, height, color, op, image)
int x, y, width, height ;
enum col_type color ;
enum op_type op ;
enum image_type image ;
{            
  if (!X->started) return ;
  X->gc_mask = GCFunction | GCStipple | GCForeground |
               GCTileStipXOrigin | GCTileStipYOrigin ;
    
/*  If this is the DEAD or SICK image, then we only want to display it once,
 *  centralised in the available area.
 */
             
  if (image == DEAD || image == SICK)
    {        
      if (width > ICONWIDTH)
        {    
          x     += (width - ICONWIDTH) / 2 ;
          width  = ICONWIDTH ;
        }    
      if (height > ICONHEIGHT)
        {
          y     += (height - ICONHEIGHT) / 2 ;
          height = ICONHEIGHT ;
        }    
    }        
 
  if (v->iscolor == FALSE || color == C_FOREGND)
    X->gc_val.foreground = X->foregnd ;
  else X->gc_val.foreground = X->palette[(int) color] ;

  X->gc_val.function    = X->opvals[(int) op] ;
  X->gc_val.stipple     = X->images[(int) image] ;
  X->gc_val.ts_x_origin = x ;
  X->gc_val.ts_y_origin = y ;
  XChangeGC(X->dpy, X->ropgc, X->gc_mask, &X->gc_val) ;
  XFillRectangle(X->dpy, X->wid, X->ropgc, x, y, width, height) ;
}


void
draw_text(x, y, op, str)
int x, y ;
enum op_type op ;
char *str ;
{
  if (!X->started) return ;
  X->gc_val.foreground = X->foregnd ;
  X->gc_val.function   = X->opvals[(int) op] ;

#ifdef OW_I18N
  if (X->multi_byte)
    {
      XChangeGC(X->dpy, X->gc, GCForeground | GCFunction, &X->gc_val) ;
      XmbDrawImageString(X->dpy, X->wid, X->font_set, X->gc, 
		         x, y + BORDER + X->ch_ascent, str, strlen(str)) ;
    }
  else
#endif /*OW_I18N*/
    {
      X->gc_val.font = X->font->fid ;
      XChangeGC(X->dpy, X->gc,
                GCForeground | GCFont | GCFunction, &X->gc_val) ;
      XDrawImageString(X->dpy, X->wid, X->gc, x, y + BORDER + X->ch_ascent,
                       str, strlen(str)) ;
    }
}


Notify_value
frame_interpose(frame, event, arg, type)
Frame frame ;
Event *event ;
Notify_arg arg ;
Notify_event_type type ;
{
  Notify_value rc ;

  rc = notify_next_event_func(frame, (Notify_event) event, arg, type) ;
  if (!X->started) return(rc) ;
  if ((int) xv_get(frame, FRAME_CLOSED) == TRUE &&
      event_action(event) == WIN_REPAINT)
    {
      MEMSET((char *) v->save, 0, sizeof(int) * MAXMETERS * v->maxsave) ;
      set_timer(0) ;
    }
  return(rc) ;
}


void
get_font(size)
int size ;
{
  char fname[40] ;    /* Used to construct the required font name. */
  int i ;

#ifdef OW_I18N
  XFontSetExtents *font_extents ;
#endif /*OW_I18N*/

  if (!(X->font = ds_get_font(X->dpy, v->fontname, size)))
    {
      FPRINTF(stderr, mess[(int) M_FONT]) ;
      exit(1) ;
    }

#ifdef OW_I18N
  if (X->multi_byte)
    {
      if ((X->pf = (Xv_Font ) xv_find(NULL, FONT,
			             FONT_FAMILY,    FONT_FAMILY_SANS_SERIF,
			             FONT_STYLE,     FONT_STYLE_NORMAL,
			             0)) == 0)
        {
          FPRINTF(stderr, mess[(int) M_FONT]) ;
          exit(1) ;
        }
      else
        {
          X->font_set   = (XFontSet) xv_get(X->pf, FONT_SET_ID) ;
          font_extents  = (XFontSetExtents *) XExtentsOfFontSet(X->font_set) ;
          v->charheight = font_extents->max_logical_extent.height + BORDER ;
          v->charwidth  = font_extents->max_logical_extent.width ;

/* The max_logical_extent.y is the negative of the ascent. */

          X->ch_ascent  = -(font_extents->max_logical_extent.y) ;
        }
    }
  else
    {
      v->charheight = X->font->max_bounds.ascent +
                      X->font->max_bounds.descent + BORDER ;
      v->charwidth  = X->font->max_bounds.rbearing -
                      X->font->min_bounds.lbearing ;
      X->ch_ascent  = X->font->max_bounds.ascent ;
    }
#else
  v->charheight = X->font->max_bounds.ascent +
                  X->font->max_bounds.descent + BORDER ;
  v->charwidth  = X->font->max_bounds.rbearing - X->font->min_bounds.lbearing ;
  X->ch_ascent  = X->font->max_bounds.ascent ;
#endif /*OW_I18N*/
}


char *
get_resource(rtype)      /* Get perfmeter resource from merged database. */
enum res_type rtype ;
{
  char str[MAXSTRING] ;

  STRCPY(str, perf_res[(int) rtype]) ;
  return(ds_get_resource(X->rDB, v->appname, str)) ;
}


int
get_str_width(str)         /* Get width in pixels of string value. */
char *str ;
{
#ifdef OW_I18N
  Font_string_dims dims ;

  if (X->multi_byte)
    {
      if (X->pf == NULL) return(0) ;
      (void) xv_get(X->pf, FONT_STRING_DIMS, str, &dims) ;
      return(dims.width) ;
    }
#endif /*OW_I18N*/
  return(ds_get_strwidth(X->font, str)) ;
}


char *MSGFILE_ERROR   = "SUNW_DESKSET_PERFMETER_ERR" ;
char *MSGFILE_LABEL   = "SUNW_DESKSET_PERFMETER_LABEL" ;
char *MSGFILE_MESSAGE = "SUNW_DESKSET_PERFMETER_MSG" ;


static void
load_colors()      /* Create and load reve color map. */
{
  XColor ccol ;
  int i, numcolors ;

  if (v->iscolor)
    {
      numcolors = 0 ;
      for (i = 0; i < MAXMETERS+1; i++)
        {
          if (v->colstr[i] != NULL &&
              (XParseColor(X->dpy, DefaultColormap(X->dpy, X->screen),
                           v->colstr[i], &ccol) != 0))
            {
              if (XAllocColor(X->dpy,
                           DefaultColormap(X->dpy, X->screen), &ccol) == True)
                X->palette[numcolors++] = ccol.pixel ;
            }
          else X->palette[numcolors++] = X->foregnd ;
        }
      if (numcolors < MAXMETERS+1) v->iscolor = FALSE ;
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
make_canvas()
{
  char *tool_label ;

/* Set label on frame. We couldn't do this before we got the display. */

  if (v->titleline == NULL)
    {
      tool_label = calloc(1, (strlen(vstrs[(int) V_FLABEL]) +
                              strlen((char *) ds_relname()) +
                              strlen((char *) ds_hostname(X->dpy)) + 3)) ;
 
      SPRINTF(tool_label, "%s %s%s",
              vstrs[(int) V_FLABEL], ds_relname(), ds_hostname(X->dpy)) ;
    }
  else tool_label = v->titleline ;
 
  XV_SET(Perfmeter_frame->frame, XV_LABEL, tool_label, 0) ;
  FREE(tool_label) ;

  XV_SET(Perfmeter_frame->canvas,
         CANVAS_AUTO_EXPAND,    TRUE,
         CANVAS_AUTO_SHRINK,    TRUE,
         CANVAS_AUTO_CLEAR,     TRUE,
         CANVAS_RETAINED,       FALSE,
         XV_HELP_DATA,          hstr[(int) H_MCANVAS],
         0) ;
  X->pw = canvas_paint_window(Perfmeter_frame->canvas) ;
  XV_SET(X->pw, WIN_CONSUME_X_EVENT_MASK, VisibilityChangeMask, 0) ;
}


void
make_image(n, width, height, image)    /* Create a ropable image. */
enum image_type n ;
int width, height ;
unsigned short image[] ;
{
  Server_image temp ;

  temp = xv_create(XV_NULL,           SERVER_IMAGE,
                   XV_WIDTH,          width,
                   XV_HEIGHT,         height,
                   SERVER_IMAGE_BITS, image,
                   0) ;
  X->images[(int) n] = (int) xv_get(temp, XV_XID) ;
}


void
make_xlib_stuff()    /* Create Xlib objects for doing graphics. */
{
  Drawable xid ;

  xid = (Drawable) xv_get(Perfmeter_frame->frame, XV_XID) ;

  X->opvals[(int) GOR]  = GXor ;
  X->opvals[(int) GSRC] = GXcopy ;

  make_image(RSPEEDO, ICONWIDTH, ICONHEIGHT, ricon_data) ;
  make_image(DEAD,    ICONWIDTH, ICONHEIGHT, dead_data) ;
  make_image(SICK,    ICONWIDTH, ICONHEIGHT, sick_data) ;

  X->wid     = (Drawable) xv_get(X->pw,    XV_XID) ;
  v->iscolor = ((int) xv_get(Perfmeter_frame->frame, WIN_DEPTH) > 1) ? 1 : 0 ;

  get_font(v->fontsize) ;           /* Open font at correct scale. */

  X->screen  = DefaultScreen(X->dpy) ;
  X->cms     = (Cms) xv_get(Perfmeter_frame->frame, WIN_CMS) ;
  X->foregnd = xv_get(X->cms, CMS_FOREGROUND_PIXEL) ;
  X->backgnd = xv_get(X->cms, CMS_BACKGROUND_PIXEL) ;
  X->gc_mask = GCForeground | GCBackground | GCGraphicsExposures ;
  X->gc_val.foreground = X->foregnd ;
  X->gc_val.background = X->backgnd ;
  X->gc_val.graphics_exposures = True ;
  X->gc    = XCreateGC(X->dpy, xid, X->gc_mask, &X->gc_val) ;
  X->ropgc = XCreateGC(X->dpy, xid, X->gc_mask, &X->gc_val) ;
  XSetFillStyle(X->dpy, X->ropgc, FillOpaqueStippled) ;

  load_colors() ;                  /* Load the perfmeter colormap. */
  if (v->debug) XSynchronize(X->dpy, TRUE) ;
}


Notify_value
meter_event(pw, event, arg, type)
Xv_Window pw ;
Event *event ;
Notify_arg arg ;
Notify_event_type type ;
{
  Notify_value value ;
  XEvent *xev ;
  int id ;

  value = notify_next_event_func(pw, (Notify_event) event, arg, type) ;
  if (!X->started) return(value) ;
  id = event_action(event) ;

  if (id == WIN_VISIBILITY_NOTIFY)
    {
      xev = (XEvent *) event_xevent(event) ;

           if (xev->xvisibility.state == VisibilityPartiallyObscured)
        XSetGraphicsExposures(X->dpy, X->gc, TRUE) ;
      else if (xev->xvisibility.state == VisibilityUnobscured)
        XSetGraphicsExposures(X->dpy, X->gc, FALSE) ;

      if (v->collect_data == FALSE)
        if (xev->xvisibility.state == VisibilityFullyObscured)
          {
            MEMSET((char *) v->save, 0, sizeof(int) * MAXMETERS * v->maxsave) ;
            set_timer(0) ;
          } 
        else
          {
            set_timer(v->sampletime) ;
            meter_paint() ;
          } 
    }
  else if (id == ACTION_OPEN)
    {
      set_timer(v->sampletime) ;
      meter_paint() ; 
    }
  else if (id == ACTION_CLOSE)
    {
      MEMSET((char *) v->save, 0, sizeof(int) * MAXMETERS * v->maxsave) ;
      set_timer(0) ;
    }
  else if ((id == ACTION_MENU) && (pw == X->pw))
    menu_show(X->pm_menu, pw, event, 0) ;

  if (event_is_ascii(event) && event_is_down(event)) do_char(id) ;
  else if (id == ACTION_PROPS) pm_create_props() ;
  return(value) ;
}


/*ARGSUSED*/ 
Notify_value 
meter_itimer_expired(meter, which)
Notify_client meter ;
int which ;
{
  updatedata() ;
  meter_update() ;
  set_timer(v->sampletime) ;
  return(NOTIFY_DONE) ;
}


void
pm_create_frame_menu()     /* Create open window popup menu. */
{
  Menu_item mi ;
  int i ;

  X->pm_menu = perfmeter_pm_menu_create((caddr_t) INSTANCE, (Xv_opaque) NULL) ;

  for (i = 0; i < MAXMETERS; i++)
    {
      mi = (Menu_item) xv_get(X->pm_menu, MENU_NTH_ITEM, i+2) ;
      XV_SET(mi,
             MENU_CLIENT_DATA, i,
             MENU_ACTION_PROC, do_menu,
             XV_HELP_DATA,     pmenu_help[i],
             0) ;
    }

  mi = (Menu_item) xv_get(X->pm_menu, MENU_NTH_ITEM, MENU_PROPS) ;
  XV_SET(mi,
         MENU_ACTION_PROC, do_menu,
         MENU_CLIENT_DATA, MENU_PROPS,
#ifdef KYBDACC
	 MENU_ACCELERATOR, "coreset Props",
#endif /* KYBDACC */
         XV_HELP_DATA,     hstr[(int) H_PROPS],
         0) ;

#ifdef KYBDACC
  XV_SET(Perfmeter_frame->frame, FRAME_MENU_ADD, X->pm_menu, NULL) ;
#endif /* KYBDACC */

}


static int
item_label_width(item)
Panel_item item;
{
  char *string ;
  Font_string_dims font_size ;
  Xv_Font font ;

  font   = (Xv_Font) xv_get(item, PANEL_LABEL_FONT) ;
  string = (char *)  xv_get(item, PANEL_LABEL_STRING) ;
  (void) xv_get(font, FONT_STRING_DIMS, string, &font_size) ;
  return(font_size.width) ;
}


void
pm_create_props()
{
  Rect *r, *sr ;
  int i, lw, lx, rw, w ;
  int itrackval = 0 ;
 
  if (Perfmeter_pm_props_frame != NULL)
    {
      XV_SET(Perfmeter_pm_props_frame->pm_props_frame, XV_SHOW, TRUE, 0) ;
      return ;
    }

  Perfmeter_pm_props_frame =
    perfmeter_pm_props_frame_objects_initialize(NULL, Perfmeter_frame->frame) ;

  XV_SET(Perfmeter_pm_props_frame->pm_props_frame,
#ifdef OW_I18N
/*         WIN_USE_IM, TRUE,    XXXXXX temporary  */
#endif /*OW_I18N*/
         XV_HELP_DATA, hstr[(int) H_PFRAME],
         0) ;
 
  DS_ADD_HELP(Perfmeter_pm_props_frame->track_type,   hstr[(int) H_VCHOICE]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->dir_type,     hstr[(int) H_DIRCHOICE]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->disp_type,    hstr[(int) H_DISPCHOICE]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->machine,      hstr[(int) H_RCHOICE]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->sampletime,   hstr[(int) H_SLEN]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->hour_hand,    hstr[(int) H_HRHAND]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->minute_hand,  hstr[(int) H_MINHAND]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->stime_secs,   hstr[(int) H_SLEN]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->htime_secs,   hstr[(int) H_HRHAND]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->mtime_secs,   hstr[(int) H_MINHAND]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->graph_type,   hstr[(int) H_GCHOICE]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->machine_name, hstr[(int) H_MNAME]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->do_log,       hstr[(int) H_DOLOG]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->log_name,     hstr[(int) H_LNAME]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->apply_but,    hstr[(int) H_APPLY]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->def_but,      hstr[(int) H_DEFS]) ;
  DS_ADD_HELP(Perfmeter_pm_props_frame->reset_but,    hstr[(int) H_RESET]) ;

  XV_SET(Perfmeter_pm_props_frame->dir_type,
         PANEL_VALUE, (int) v->direction,
         0) ;
  XV_SET(Perfmeter_pm_props_frame->disp_type,
         PANEL_VALUE, (int) v->dtype,
         0) ;
  XV_SET(Perfmeter_pm_props_frame->sampletime,
         PANEL_VALUE, v->sampletime,
         0) ;
  XV_SET(Perfmeter_pm_props_frame->hour_hand,
         PANEL_VALUE, v->hourhandintv,
         0) ;
  XV_SET(Perfmeter_pm_props_frame->minute_hand,
         PANEL_VALUE, v->minutehandintv,
         0) ;

  for (i = MAXMETERS-1; i >= 0; i--)
    itrackval = (itrackval << 1) + (v->meters[i].m_showing != 0) ;

  XV_SET(Perfmeter_pm_props_frame->track_type,
         PANEL_VALUE, itrackval,
         0) ;

  XV_SET(Perfmeter_pm_props_frame->graph_type,
         PANEL_VALUE, (int) v->solid_graph,
         0) ;

  r = (Rect *) xv_get(Perfmeter_pm_props_frame->sampletime, PANEL_ITEM_RECT) ;
  XV_SET(Perfmeter_pm_props_frame->stime_secs,
         XV_X, (int) (r->r_left + r->r_width + SMALL_POFFSET),
         XV_Y, (int) r->r_top,
         0) ;

  r = (Rect *) xv_get(Perfmeter_pm_props_frame->hour_hand, PANEL_ITEM_RECT) ;
  XV_SET(Perfmeter_pm_props_frame->htime_secs,
         XV_X, (int) (r->r_left + r->r_width + SMALL_POFFSET),
         XV_Y, (int) r->r_top,
         0) ;

  r = (Rect *) xv_get(Perfmeter_pm_props_frame->minute_hand, PANEL_ITEM_RECT) ;
  XV_SET(Perfmeter_pm_props_frame->mtime_secs,
         XV_X, (int) (r->r_left + r->r_width + SMALL_POFFSET),
         XV_Y, (int) r->r_top,
         0) ;

/*  Layout the four panel items on the right hand side of the property sheet.
 *  Work out the width of the longest panel "item" on the left side of the
 *  property sheet. Create the right hand property items, with correct row
 *  values. Work out the length of the longest label for the right hand items.
 *  Position right items appropriately.
 */

  lw = 0 ;
  r = (Rect *) xv_get(Perfmeter_pm_props_frame->dir_type, PANEL_ITEM_RECT) ;
  if (r->r_width > lw)
    {
      lx = r->r_left ;
      lw = r->r_width ;
    }

  r = (Rect *) xv_get(Perfmeter_pm_props_frame->disp_type, PANEL_ITEM_RECT) ;
  if (r->r_width > lw)
    {
      lx = r->r_left ;
      lw = r->r_width ;
    }

  r = (Rect *) xv_get(Perfmeter_pm_props_frame->machine, PANEL_ITEM_RECT) ; 
  if (r->r_width > lw)
    {
      lx = r->r_left ;
      lw = r->r_width ;
    }

  r  = (Rect *) xv_get(Perfmeter_pm_props_frame->sampletime, PANEL_ITEM_RECT) ;
  sr = (Rect *) xv_get(Perfmeter_pm_props_frame->stime_secs, PANEL_ITEM_RECT) ;
  if ((r->r_width + sr->r_width) > lw)
    {
      lx = r->r_left ;
      lw = r->r_width + sr->r_width ;
    }

  r  = (Rect *) xv_get(Perfmeter_pm_props_frame->sampletime, PANEL_ITEM_RECT) ;
  sr = (Rect *) xv_get(Perfmeter_pm_props_frame->htime_secs, PANEL_ITEM_RECT) ;
  if ((r->r_width + sr->r_width) > lw)
    {
      lx = r->r_left ;
      lw = r->r_width + sr->r_width ; 
    }

  r  = (Rect *) xv_get(Perfmeter_pm_props_frame->sampletime, PANEL_ITEM_RECT) ;
  sr = (Rect *) xv_get(Perfmeter_pm_props_frame->mtime_secs, PANEL_ITEM_RECT) ;
  if ((r->r_width + sr->r_width) > lw)
    {
      lx = r->r_left ;
      lw = r->r_width + sr->r_width ; 
    }

  rw = 0 ;
  if ((w = item_label_width(Perfmeter_pm_props_frame->graph_type))   > rw)
    rw = w ;
  if ((w = item_label_width(Perfmeter_pm_props_frame->machine_name)) > rw)
    rw = w ;
  if ((w = item_label_width(Perfmeter_pm_props_frame->do_log))       > rw)
    rw = w ;
  if ((w = item_label_width(Perfmeter_pm_props_frame->log_name))     > rw)
    rw = w ;

  XV_SET(Perfmeter_pm_props_frame->graph_type,
         PANEL_VALUE_X, lx + lw + LARGE_POFFSET + rw,
         XV_Y, (int) xv_get(Perfmeter_pm_props_frame->disp_type, XV_Y),
         0) ;
  XV_SET(Perfmeter_pm_props_frame->machine_name,
         PANEL_VALUE_X, lx + lw + LARGE_POFFSET + rw,
         XV_Y, (int) xv_get(Perfmeter_pm_props_frame->machine, XV_Y),
         0) ;
  XV_SET(Perfmeter_pm_props_frame->do_log,
         PANEL_VALUE_X, lx + lw + LARGE_POFFSET + rw,
         XV_Y, (int) xv_get(Perfmeter_pm_props_frame->hour_hand, XV_Y),
         0) ;
  XV_SET(Perfmeter_pm_props_frame->log_name,
         PANEL_VALUE_X, lx + lw + LARGE_POFFSET + rw,
         XV_Y, (int) xv_get(Perfmeter_pm_props_frame->minute_hand, XV_Y),
         0) ; 

  v->isprops = TRUE ;
  pm_display_and_locate_props() ;

  (void) ds_center_items(Perfmeter_pm_props_frame->pm_props_panel, -1,
                         Perfmeter_pm_props_frame->apply_but,
                         Perfmeter_pm_props_frame->def_but,
                         Perfmeter_pm_props_frame->reset_but,
                         0) ;
  ds_position_popup(Perfmeter_frame->frame,
                    Perfmeter_pm_props_frame->pm_props_frame, DS_POPUP_LOR) ;
  if (X->started)
    XV_SET(Perfmeter_pm_props_frame->pm_props_frame, XV_SHOW, TRUE, 0) ;
}


void
pm_display_and_locate_props()
{
  int i ;
  int itrackval = 0 ;

  for (i = MAXMETERS-1; i >= 0; i--)
    itrackval = (itrackval << 1) + (v->meters[i].m_showing != 0) ;
  adjust_track_val(itrackval) ;

  item_set_val(IMACH, v->remote) ;
  XV_SET(Perfmeter_pm_props_frame->machine_name,
         PANEL_INACTIVE, !v->remote,
         PANEL_VALUE,     v->hostname,
         0) ;
 
  item_set_val(ILOG, v->save_log) ;
  XV_SET(Perfmeter_pm_props_frame->log_name, PANEL_INACTIVE, !v->save_log, 0) ;
  if (v->sname != NULL)
    XV_SET(Perfmeter_pm_props_frame->log_name, PANEL_VALUE, v->sname, 0) ;

  item_set_val(IDIR,   (int) v->direction) ;
  item_set_val(ITYPE,  (int) v->dtype) ;
  item_set_val(IGRAPH, (int) v->solid_graph) ;
  XV_SET(Perfmeter_pm_props_frame->graph_type,
         PANEL_INACTIVE, !xv_get(Perfmeter_pm_props_frame->disp_type,
                                 PANEL_VALUE),
         0) ;

  item_set_val(ISTIME, v->sampletime) ;
  item_set_val(IHHAND, v->hourhandintv) ;
  item_set_val(IMHAND, v->minutehandintv) ; 
  window_fit(Perfmeter_pm_props_frame->pm_props_panel) ;
  window_fit(Perfmeter_pm_props_frame->pm_props_frame) ;
}


/*ARGSUSED*/
void
pm_display_choice_proc(panel_item, value, event)
Panel_item  panel_item ;
int value ;
Event *event ;
{
  XV_SET(Perfmeter_pm_props_frame->graph_type, PANEL_INACTIVE, !value, 0) ;
}


/*ARGSUSED*/
void
pm_log_choice_proc(panel_item, value, event)
Panel_item panel_item ;
int value ;
Event *event ;
{
  if (Perfmeter_pm_props_frame == NULL)
	return;
  XV_SET(Perfmeter_pm_props_frame->log_name, PANEL_INACTIVE, !value, 0) ;
  if (value)
    XV_SET(Perfmeter_pm_props_frame->pm_props_panel,
           PANEL_CARET_ITEM, Perfmeter_pm_props_frame->log_name,
           0) ;
}


/*ARGSUSED*/
void
pm_machine_choice_proc(panel_item, value, event)
Panel_item  panel_item ;
int value ;
Event *event ;
{
  XV_SET(Perfmeter_pm_props_frame->machine_name, PANEL_INACTIVE, !value, 0) ;
  if (value)
    XV_SET(Perfmeter_pm_props_frame->pm_props_panel,
           PANEL_CARET_ITEM, Perfmeter_pm_props_frame->machine_name,
           0) ;
}


/*ARGSUSED*/
Notify_value
pm_props_apply_proc(item, event)
Panel_item item ;
Event *event ;
{  
  X->cur_item = item ;
  set_prop_vals(item_get_str(IMACHNAME), v->dtype, v->direction,
                v->dlen, v->remote, v->width) ;
  return(NOTIFY_DONE) ;
}


/*ARGSUSED*/
Notify_value
pm_props_default_proc(item, event)
Panel_item item ;
Event *event ;
{
  pm_props_apply_proc(item, event) ;
  write_resources() ;
  return(NOTIFY_DONE) ;
}


/*ARGSUSED*/
Notify_value
pm_props_reset_proc(item, event)
Panel_item item ;
Event *event ;
{
  pm_display_and_locate_props() ;
  XV_SET(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0) ;
  return(NOTIFY_DONE) ;
}


void
put_resource(rtype, value)  /* Put perfmeter resource into deskset database. */
enum res_type rtype ;
char *value ;
{
  ds_put_resource(&X->desksetDB, v->appname, perf_res[(int) rtype], value) ;
}


void
save_resources()
{
  ds_save_resources(X->desksetDB) ;
}


/* Resize area based on old and new sizes, and old and new direction. */

void
resize_display(area, oldlen, newlen, olddir, newdir)
enum area_type area ;
int oldlen, newlen ;
enum direction_type olddir, newdir ;
{
  int h, temp, w ;
  Rect *r ;

  r = (Rect *) xv_get(Perfmeter_frame->frame, FRAME_OPEN_RECT) ;
  switch (area)
    {
      case DIAL  : set_dial_dims(&w, &h, newlen) ;
                   r->r_width = w ;
                   r->r_height = h ;
                   break ;

      case GRAPH : if (v->resize_graph == FALSE)
                     {
                       set_dims(r->r_width, r->r_height) ;
                       repaint_from_cache(r->r_left,  r->r_top,
                                          r->r_width, r->r_height) ;
                       return ;
                     }
                   if (oldlen == newlen)
                     {
                       if (olddir != newdir)
                         {
                           temp      = v->width ;
                           v->width  = v->height ;
                           v->height = temp ;
                         }
                       if (newdir == HORIZONTAL)
                         {
                           r->r_width = (v->width * newlen) +
                                        (GRAPHGAP * (newlen-1)) +
                                        2 * BORDER + 1 ;
                           r->r_height = v->height + v->charheight +
                                         2*BORDER + 1 ;
                           if (v->remote) r->r_height += v->charheight ;
                         }
                       else
                         {
                           r->r_width = v->width + 2*BORDER + 1 ;
                           r->r_height = (v->height + v->charheight) * newlen +
                                         2*BORDER + 1 ;
                           if (v->remote) r->r_height += v->charheight ;
                         }
                     }
                   else
                     {
                       if (olddir != newdir)
                         {
                           temp         = v->winwidth ;
                           v->winwidth  = v->winheight ;
                           v->winheight = temp ;
                         }
                       if (newdir == HORIZONTAL)
                         {
                           w = v->winwidth - 2*BORDER - 1 ;
                           w -= (GRAPHGAP * (oldlen - 1)) ;
                           w /= oldlen ;
                           r->r_width = (w * newlen) +
                                        (GRAPHGAP * (newlen - 1)) +
                                         2*BORDER + 1 ;
                           r->r_height = v->winheight ;
                         }
                       else
                         {
                           r->r_width = v->winwidth ;
                           h = v->winheight - 2*BORDER - 1 ;
                           h -= v->charheight * oldlen ;
                           if (v->remote) h -= v->charheight ;
                           h /= oldlen ;
                           r->r_height = (h + v->charheight) * newlen +
                                          2*BORDER + 1 ;
                         }
                     }
    }
  XV_SET(Perfmeter_frame->frame, FRAME_OPEN_RECT, r, 0) ;
}


void
save_cmdline(argc, argv)
int argc ;
char *argv[] ;
{
  DS_SAVE_CMDLINE(Perfmeter_frame->frame, argc, argv) ;
}


void
set_frame_hints()     /* Setup minimum and maximum frame size hints. */
{
  int maxh, maxw, minh, minw ;

  if (X->repaint == FALSE) return ;   /* Is the poor window withdrawn? */

  if (v->dtype == DIAL)
    {
      minw = ((v->direction == HORIZONTAL) ? v->dlen : 1) * ICONWIDTH ;

      minh =   ((v->direction == HORIZONTAL) ? 1 : v->dlen) * ICONHEIGHT ;
      minh += (((v->direction == HORIZONTAL) ? 1 : v->dlen) * v->charheight) ;
      if (v->remote) minh += v->charheight ;
      maxw = minw ;
      maxh = minh ;
    }
  else
    {
      minh = minw = 1 ;
      maxw = DisplayWidth(X->dpy, X->screen) ;
      maxh = DisplayHeight(X->dpy, X->screen) ;
    }

  XV_SET(Perfmeter_frame->frame,
         FRAME_MIN_SIZE, minw, minh,
         FRAME_MAX_SIZE, maxw, maxh,
         0) ;
}


void         
set_timer(n) 
int n ;
{
  struct itimerval itimer ;
    
  itimer = NOTIFY_NO_ITIMER ;
  itimer.it_value.tv_sec = n ;
  NOTIFY_SET_ITIMER_FUNC((Notify_client) &X->meter_client,
                         n > 0 ? (Notify_func) meter_itimer_expired :
                                 NOTIFY_FUNC_NULL,
                         ITIMER_REAL, &itimer, (struct itimerval *) 0) ;
}            
             
 
void         
shift_left(x, y, width, height)   /* Shift window display one pixel left. */
int x, y, width, height ;
{
  if (!X->started) return ;
  if (height < 0) return ;
  XCopyArea(X->dpy, X->wid, X->wid, X->gc, x+1, y, width, height, x, y) ;
}


void
start_events()      /* Assign event handlers to the two drawing surfaces. */
{
  NOTIFY_INTERPOSE_EVENT_FUNC((Notify_client) Perfmeter_frame->frame,
                              meter_event, NOTIFY_SAFE) ;
  NOTIFY_INTERPOSE_DESTROY_FUNC(Perfmeter_frame->frame, destroy_tool) ;
}


void
start_tool()       /* Set itimer event handler and show window. */
{
  METER_ITIMER_EXPIRED((Notify_client) &X->meter_client, ITIMER_REAL) ;
  X->started = 1 ;
  xv_main_loop(Perfmeter_frame->frame) ;

  while (v->want_redisplay)
    {
      v->want_redisplay = 0 ;
      meter_paint() ;
      notify_start() ;
    }
}


static void
xerror_interpose(display, error)
Display *display ;
XErrorEvent *error ;
{
  char msg[80] ;

  XGetErrorText(display, error->error_code, msg, 80) ;
  FPRINTF(stderr, DGET("\nX Error (intercepted): %s\n"), msg) ;
  FPRINTF(stderr, DGET("Major Request Code   : %d\n"),   error->request_code) ;
  FPRINTF(stderr, DGET("Minor Request Code   : %d\n"),   error->minor_code) ;
  FPRINTF(stderr, DGET("Resource ID (XID)    : %u\n"),   error->resourceid) ;
  FPRINTF(stderr, DGET("Error Serial Number  : %u\n"),   error->serial) ;
  abort() ;
}


char *
xv_item_get_str(itype)      /* Get property item string value. */
enum item_type itype ;
{
  switch (itype)
    {
      case IMACHNAME : return((char *)
                              xv_get(Perfmeter_pm_props_frame->machine_name,
                                     PANEL_VALUE)) ;
      case ILOGNAME  : return((char *)
                              xv_get(Perfmeter_pm_props_frame->log_name,
                                     PANEL_VALUE)) ;
    }
/*NOTREACHED*/
}


int
xv_item_get_val(itype)    /* Get property item integer value. */
enum item_type itype ;
{
  switch (itype)
    {
      case ITRACK  : return((int) xv_get(Perfmeter_pm_props_frame->track_type,
                                         PANEL_VALUE)) ;
      case IDIR    : return((int) xv_get(Perfmeter_pm_props_frame->dir_type,
                                         PANEL_VALUE)) ;
      case ITYPE   : return((int) xv_get(Perfmeter_pm_props_frame->disp_type,
                                         PANEL_VALUE)) ;
      case IGRAPH  : return((int) xv_get(Perfmeter_pm_props_frame->graph_type,
                                         PANEL_VALUE)) ;
      case IMACH   : return((int) xv_get(Perfmeter_pm_props_frame->machine,
                                         PANEL_VALUE)) ;
      case ISTIME  : return((int) xv_get(Perfmeter_pm_props_frame->sampletime,
                                         PANEL_VALUE)) ;
      case IHHAND  : return((int) xv_get(Perfmeter_pm_props_frame->hour_hand,
                                         PANEL_VALUE)) ;
      case ILOG    : return((int) xv_get(Perfmeter_pm_props_frame->do_log,
                                         PANEL_VALUE)) ;
      case IMHAND  : return((int) xv_get(Perfmeter_pm_props_frame->minute_hand,
                                         PANEL_VALUE)) ;
    }
/*NOTREACHED*/
}

void
xv_item_set_str(itype, str)    /* Set property item str value. */
enum item_type itype ;
char* str ;   
{
  switch (itype) 
    {
      case ILOGNAME : 
			if (Perfmeter_pm_props_frame)
				XV_SET(Perfmeter_pm_props_frame->log_name,
                           		PANEL_VALUE, str,
                           		0) ;
                    	break ;   
    } 
}

void
xv_item_set_val(itype, value)    /* Set property item integer value. */
enum item_type itype ;
int value ;
{
  switch (itype)
    {
      case ITRACK : XV_SET(Perfmeter_pm_props_frame->track_type,
                           PANEL_VALUE, value,
                           0) ;
                    break ;
      case IDIR   : XV_SET(Perfmeter_pm_props_frame->dir_type,
                           PANEL_VALUE, value,
                           0) ;
                    break ;
      case ITYPE  : XV_SET(Perfmeter_pm_props_frame->disp_type,
                           PANEL_VALUE, value,
                           0) ;
                    break ;
      case IGRAPH : XV_SET(Perfmeter_pm_props_frame->graph_type,
                           PANEL_VALUE, value,
                           0) ;
                    break ;
      case IMACH  : XV_SET(Perfmeter_pm_props_frame->machine,
                           PANEL_VALUE, value,
                           0) ;
                    XV_SET(Perfmeter_pm_props_frame->machine_name,
                           PANEL_INACTIVE, !value,
                           0) ;
                    break ;
      case ISTIME : XV_SET(Perfmeter_pm_props_frame->sampletime,
                           PANEL_VALUE, value,
                           0) ;
                    break ;
      case IHHAND : XV_SET(Perfmeter_pm_props_frame->hour_hand,
                           PANEL_VALUE, value,
                           0) ;
                    break ;
      case ILOG   : XV_SET(Perfmeter_pm_props_frame->do_log,
                           PANEL_VALUE, value,
                           0) ;
                    break ;
      case IMHAND : XV_SET(Perfmeter_pm_props_frame->minute_hand,
                           PANEL_VALUE, value,
                           0) ;
    }
}
