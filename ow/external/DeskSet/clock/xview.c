#ifndef lint
static char sccsid[]="@(#)xview.c	1.27 01/10/96 Copyright 1988-1990 Sun Microsystem, Inc." ;
#endif

/*
 *  The DeskSet clock    (graphics dependent routines).
 *
 *  Copyright (c) 1988-1990  Sun Microsystems, Inc.  
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in posesion of this copy.
 *
 *  RESTRICTED RIGHTS LEGEND:  Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <euc.h>
#include <tzfile.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "clock.h"

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <xview/font.h>
#include <xview/canvas.h>
#include <xview/notify.h>
#include <xview/svrimage.h>
#include <xview/xv_xrect.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include "ds_xlib.h"
#include "ds_popup.h"

#include "clockui.h"

#define  DS_ADD_HELP                  (void) ds_add_help
#define  DS_GET_FRAME_SIZE            (void) ds_get_frame_size
#define  DS_SAVE_CMDLINE              (void) ds_save_cmdline
#define  DS_SET_FRAME_SIZE            (void) ds_set_frame_size

#define  NOTIFY_INTERPOSE_EVENT_FUNC  (void) notify_interpose_event_func
#define  NOTIFY_SET_ITIMER_FUNC       (void) notify_set_itimer_func
#define  NOTIFY_SET_SIGNAL_CHECK      (void) notify_set_signal_check
#define  NOTIFY_SET_WAIT3_FUNC        (void) notify_set_wait3_func
#define  XV_CREATE                    (void) xv_create
#define  XV_DESTROY                   (void) xv_destroy
#define  XV_GET                       (void) xv_get
#define  XV_SET                       (void) xv_set

const char *WORKSPACE_CLASS_RES =	"OpenWindows.WorkspaceColor";
const char *WORKSPACE_RES =       	"openwindows.workspacecolor";
const char *DEFFONT = 			"fixed";

#define  MENU_PROPS           1   /* Index into clock menu. */

#include "clock.xbm"
#include "clock.rom.xbm"

static const unsigned char gray_bits[] = {
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa
};


typedef struct Xobject {           /* XView/Xlib graphics object. */
  Menu                menu ;

  Drawable    ixid[MAXIMAGES] ;   /* Xlib handles for number images. */
  Xv_font     font[MAXFONTS] ;   /* Xlib handle to the fonts. */
  Display *dpy ;                   /* Display id of the clock frames. */
  Drawable xid[MAXSVTYPES] ;       /* Xids for all frames and images. */
  GC gc ;                          /* Main drawing graphics context. */
  GC iconsolidgc ;                 /* Foreground color for icon */
  GC iconxorgc ;                   /* XOR drawing for icon */
  GC linegc ;                      /* Line GC for window. */
  GC ropgc ;                       /* Graphics context for ransterops. */
  GC svgc ;                        /* Main GC for 1 bit deep images. */
  GC svlinegc ;                    /* Line drawing GC for 1 bit deep images. */
  GC svropgc ;                     /* Rop GC for 1 bit deep images. */
  GC svtilegc ;                    /* Tile GC for 1 bit deep images. */
  XPoint dummyA[4] ;               /* To stored scaled digital "chars". */
  XPoint dummyB[4] ;
  XPoint dummyC[4] ;
  XPoint dummyD[4] ;
  XPoint dummyE[4] ;
  XPoint dummyF[4] ;
  XPoint dummyG[4] ;
  XPoint dummyH[4] ;
  XPoint dummyI[4] ;
  XPoint dummyJ[4] ;
  XPoint dummyK[4] ;
  XPoint dummyL[4] ;
  XPoint dummyM[4] ;
  XPoint dummyN[4] ;
  XPoint dummyO[6] ;
  XPoint *workingPoints[MAXDIGCHARS] ;
  XrmDatabase desksetDB ;          /* Deskset resources database. */
  XrmDatabase rDB ;                /* Combined resources database. */
  int opvals[MAXOPS] ;             /* Rasterop values used. */
  int firstpaint ;                 /* TRUE for first valid canvas repaint. */
  int latest_analog_w ;            /* Don't redo init_analog() if match. */
  int latest_analog_h ;            /* Don't redo init_analog() if match. */
  int repaint ;                    /* Set in clock_repaint_proc when ready. */
  unsigned int depth ;
  unsigned long backgnd ;          /* Default background color. */
  unsigned long foregnd ;          /* Default foreground color. */
  unsigned long wscolor ;          /* Workspace color (icon background). */
} XObject ;

typedef struct Xobject *ClockXlib ;


       void clock_repaint_proc        P((Canvas, Xv_Window, Display *,
                                         Xv_Window, Xv_xrectlist *)) ;
static void menu_choose               P((Menu, Menu_item)) ;
       void stopwatch_proc            P((Panel_item, int, Event *)) ;
static void test_prop_width           P((Xv_opaque, int *)) ;
       void timezone_choice_proc      P((Panel_item, int, Event *)) ;
static void xerror_interpose          P((Display *, XErrorEvent *)) ;

static Menu gen_tzmenu                P((char *)) ;
static Menu make_menu                 P(()) ;

Menu_item show_props           P((Menu_item, Menu_generate)) ;

static void make_image        P((enum sv_type, int, int)) ;

Notify_value canvas_interpose  P((Xv_Window, Event *,
                                         Notify_arg, Notify_event_type)) ;
void 	clock_apply       P((Panel_item, Event *)) ;
void 	clock_defaults    P((Panel_item, Event *)) ;
void	clock_reset       P((Panel_item , Event *)) ;
void	clock_resize_proc P((Canvas, int, int)) ;

Notify_value frame_interpose   P((Frame, Event *,
                                         Notify_arg, Notify_event_type)) ;
static Notify_value icon_interpose    P((Icon, Event *,
                                         Notify_arg, Notify_event_type)) ;
extern Notify_value timer_expired P((Notify_value, int)) ;

const XPoint partA[4] = { {  1,  0 }, {  3,  2 }, { 15,  2 }, { 17,  0 } } ;
const XPoint partB[4] = { {  0,  0 }, {  2,  2 }, {  2, 10 }, {  0, 12 } } ;
const XPoint partC[4] = { { 18,  0 }, { 16,  2 }, { 16, 10 }, { 18, 12 } } ;
const XPoint partD[4] = { {  0, 12 }, {  2, 14 }, {  2, 22 }, {  0, 24 } } ;
const XPoint partE[4] = { { 18, 12 }, { 16, 14 }, { 16, 22 }, { 18, 24 } } ;
const XPoint partF[4] = { {  1, 24 }, {  3, 22 }, { 15, 22 }, { 17, 24 } } ;
const XPoint partG[4] = { { 12,  0 }, { 10,  2 }, { 10, 10 }, { 12, 12 } } ;
const XPoint partH[4] = { { 12, 12 }, { 10, 14 }, { 10, 22 }, { 12, 24 } } ;
const XPoint partI[4] = { {  0,  0 }, {  2,  2 }, { 15,  2 }, { 17,  0 } } ;
const XPoint partJ[4] = { {  1, 24 }, {  3, 22 }, { 16, 22 }, { 18, 24 } } ;
const XPoint partK[4] = { {  0, 24 }, {  2, 22 }, { 15, 22 }, { 17, 24 } } ;
const XPoint partL[4] = { {  1,  0 }, {  3,  2 }, { 16,  2 }, { 18,  0 } } ;
const XPoint partM[4] = { { 10,  6 }, { 12,  8 }, { 10, 10 }, {  8,  8 } } ;
const XPoint partN[4] = { { 10, 14 }, { 12, 16 }, { 10, 18 }, {  8, 16 } } ;
const XPoint partO[6] = { {  1, 12 }, {  2, 11 }, { 16, 11 }, { 17, 12 },
			      { 16, 13 }, {  2, 13 } } ;

const XPoint *defaultPoints[] = {
  partA, partB, partC, partD, partE, partF, partG, partH,
  partI, partJ, partK, partL, partM, partN, partO
} ;

ClockXlib X ;

Attr_attribute INSTANCE ;
clock_frame_objects *Clock_frame ;
clock_props_objects *Clock_props ;

extern Clock clk ;
extern Options options ;
extern ClockDisplay display ;

extern const int numpos[] ;
extern const int numvals[] ;
extern const int scaleH[] ;

extern const char *clk_res[] ;           /* Valid clock resource strings. */
extern const char *hstr[] ;              /* Help messages. */
extern const char *mess[] ;              /* Messages. */
extern const char *vstrs[] ;             /* Various. */

const char *MSGFILE_ERROR   = "SUNW_DESKSET_CLOCK_ERR" ;
const char *MSGFILE_LABEL   = "SUNW_DESKSET_CLOCK_LABEL" ;
const char *MSGFILE_MESSAGE = "SUNW_DESKSET_CLOCK_MSG" ;

/*  The current scale will determine the size of the fonts used for
 *  displays the seconds values and the date/time zone information.
 */

const int  Sfscale[MAXSCALES] = { 
    WIN_SCALE_SMALL,
    WIN_SCALE_MEDIUM,
    WIN_SCALE_LARGE,
    WIN_SCALE_EXTRALARGE 
} ;

const int DTfscale[MAXSCALES] = { 
    WIN_SCALE_SMALL,
    WIN_SCALE_SMALL,
    WIN_SCALE_LARGE,
    WIN_SCALE_EXTRALARGE 
};



int
main(argc, argv)
int argc ;
char **argv ;
{
  char bind_home[MAXPATHLEN] ;

  ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home) ;
  bindtextdomain(MSGFILE_ERROR,   bind_home) ;
  bindtextdomain(MSGFILE_LABEL,   bind_home) ;
  bindtextdomain(MSGFILE_MESSAGE, bind_home) ;

  clk     = (Clock)        LINT_CAST(calloc(1, sizeof(ClockObject))) ;
  options = (Options)      LINT_CAST(calloc(1, sizeof(ClockOptions))) ;
  display = (ClockDisplay) LINT_CAST(calloc(1, sizeof(DisplayInfo))) ;
  X       = (ClockXlib)    LINT_CAST(calloc(1, sizeof(XObject))) ;

  xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
          XV_USE_LOCALE,         TRUE,
          XV_X_ERROR_PROC,       xerror_interpose,
          0) ;
  INSTANCE    = xv_unique_key() ;
  Clock_frame = clock_frame_objects_initialize(NULL, NULL) ;
  Clock_props = NULL ;

  xv_set(Clock_frame->frame,
	 WIN_CONSUME_EVENTS, WIN_VISIBILITY_NOTIFY, NULL,
	 NULL);

  /* want repaints on all resize events */
  xv_set(Clock_frame->canvas, CANVAS_FIXED_IMAGE, FALSE, NULL);
	 
  do_clock(argc, argv) ;
  exit(0) ;
/*NOTREACHED*/
}


void
beep()
{
  ds_beep(X->dpy) ;
}


void
build_numbers(width, height)
int width, height ;
{
  int i, j, n ;

  XSetFunction(X->dpy, X->svgc, GXclear) ;
  for (i = 0; i < MAXIMAGES; i++)
    {
      if (X->ixid[i] != NULL) 
	  XFreePixmap(X->dpy, X->ixid[i]);
      X->ixid[i] = XCreatePixmap(X->dpy, RootWindow(X->dpy, DefaultScreen(X->dpy)),
				 width, height, 1/* depth */);
      XCopyArea(X->dpy, X->ixid[i], X->ixid[i], X->svgc,
                0, 0, width, height, 0, 0) ;
    }

  XSetFunction(X->dpy, X->svgc, GXset) ;
  for (i = 0; i < 11; i++)
    for (j = 0; j < numvals[numpos[i]]; j++)
      {
        n = numvals[numpos[i] + j + 1] ;
        XFillPolygon(X->dpy, X->ixid[i], X->svgc, &X->workingPoints[n][0],
                     (n == 14) ? 6 : 4, Convex, CoordModeOrigin) ;
      }
}


Notify_value
canvas_interpose(pw, event, arg, type)
Xv_Window pw ;
Event *event ;
Notify_arg arg ;
Notify_event_type type ;
{
  Menu m ;
  Notify_value rc ;
  XEvent *xev ;
  int id ;
 
  rc = notify_next_event_func(pw, (Notify_event) event, arg, type) ;
  if (!display->started) return(rc) ;
  options->doingrepaint = TRUE ;
  id = event_action(event) ;
 
  if (options->dosleep && id == WIN_VISIBILITY_NOTIFY)
    {
      xev = (XEvent *) event_xevent(event) ;
      if (xev->xvisibility.state == VisibilityFullyObscured)
        set_timer(0, 0, 0, 0) ;
      else
        {
          handle_timer() ;
          handle_time(options->face) ;
        }
    }
  else if (event_is_ascii(event) && event_is_down(event)) do_char(id) ;
  else
    switch (id)
      {
        case ACTION_PROPS : display_prop_sheet() ;
                            break ;
        case ACTION_MENU  : m = make_menu() ;
                            menu_show(m, pw, event, 0) ;
      }
  options->doingrepaint = FALSE ;
  return(rc) ;
}


void
check_args()         /* Check for font, size, label, scale and position. */
{
  char *tempstr ;

  if (defaults_exists("font.name", "Font.Name"))
    {
      tempstr = (char *) defaults_get_string("font.name",
                                             "Font.Name", DEFFONT) ;
      read_str(&display->fontnames[(int) DTFONT], tempstr) ;
      read_str(&display->fontnames[(int) SFONT],  tempstr) ;
    }

  display->clock_usersetsize = FALSE ;
  if (defaults_exists("window.width", "Window.Width"))
    {
      display->iwidth = defaults_get_integer("window.width",
                                             "Window.Width", 1) ;
      if (display->iwidth < 0) display->iwidth = -1 ;
    } 
  if (defaults_exists("window.height", "Window.Height"))
    {
      display->iheight = defaults_get_integer("window.height",
                                              "Window.Height", 1) ;
      if (display->iheight < 0) display->iheight = -1 ;
    } 
  if (display->iwidth != -1 && display->iheight != -1)
    display->clock_usersetsize = TRUE ;

  if (defaults_exists("window.scale", "Window.Scale"))
    {
      tempstr = (char *) defaults_get_string("window.scale",
                                             "Window.Scale", "medium") ;
      get_font_scale(tempstr) ;
    }
  if (defaults_exists("window.header", "Window.Header"))
    {
      tempstr = (char *) defaults_get_string("window.header",
                                             "Window.Header", "clock") ;
      if (tempstr == NULL || *tempstr == '\0') tempstr = " " ;
      read_str(&options->titleline, tempstr) ;
    }
  if (defaults_exists("window.geometry", "Window.Geometry"))
    {
      tempstr = (char *) defaults_get_string("window.geometry",
                                             "Window.Geometry", "") ;
      ds_get_geometry_size(tempstr, &display->iwidth, &display->iheight) ;
      if (display->iwidth != -1 && display->iheight != -1)
        display->clock_usersetsize = TRUE ;
    }
  if (defaults_exists("icon.pixmap", "Icon.Pixmap"))
    {
      tempstr = (char *) defaults_get_string("icon.pixmap",
                                             "Icon.Pixmap", "") ;
      if (tempstr != NULL && *tempstr != '\0' && access(tempstr, R_OK) == 0)
        read_str(&options->iconpath, tempstr) ;
      else
        {
          FPRINTF(stderr, mess[(int) M_ICON], display->progname, tempstr) ;
          xv_usage(vstrs[(int) V_LABEL]) ;
          exit(1) ;
        }
    }
  if (defaults_exists("icon.footer", "Icon.Footer"))
    {
      tempstr = (char *) defaults_get_string("icon.footer", "Icon.Footer", "") ;
      if (tempstr && *tempstr == '\0') tempstr = NULL ;
      read_str(&options->iconlabel, tempstr) ;
    }
}


/*ARGSUSED*/
void
clock_apply(item, event)
     Panel_item item ;
     Event *event ;
{
  char cval ;
  int olddate, oldtzval ;

  options->face      = (Face) xv_get(Clock_props->faceChoice, PANEL_VALUE) ;
  options->savedFace = options->face ;
 
  options->doingprops = TRUE ;
  options->icontype = (IconType) xv_get(Clock_props->iconChoice, PANEL_VALUE) ;
  options->digtype  = (DigType)  xv_get(Clock_props->digChoice,  PANEL_VALUE) ;
  options->seconds  = (int) xv_get(Clock_props->secondsToggle, PANEL_VALUE) ;
  options->achoice  = (RepType)  xv_get(Clock_props->repeat,     PANEL_VALUE) ;

  if ( USE_SFONT(options) )
      get_font(Sfscale[(int) display->OLscale],  display->OLscale, SFONT) ;

  cval = (char) xv_get(Clock_props->hrval, PANEL_VALUE) ;
  if (cval == ' ') options->ahrval = -1 ;
  else options->ahrval = (int) xv_get(Clock_props->hrval, PANEL_VALUE) ;

  cval = (char) xv_get(Clock_props->minval, PANEL_VALUE) ;
  if (cval == ' ') options->aminval = -1 ;
  else options->aminval = (int) xv_get(Clock_props->minval, PANEL_VALUE) ;

  read_str(&options->tzname,
           (char *) xv_get(Clock_props->tzname,   PANEL_VALUE)) ;
  read_str(&options->acmd,
           (char *) xv_get(Clock_props->acommand, PANEL_VALUE)) ;
  read_str(&options->hcmd,
           (char *) xv_get(Clock_props->hcommand, PANEL_VALUE)) ;
 
  olddate = options->date ;
  options->date = (int) xv_get(Clock_props->dateToggle, PANEL_VALUE) ;

  oldtzval = options->tzval ;
  options->tzval = (int) xv_get(Clock_props->timezone, PANEL_VALUE) ;

  if ( USE_DTFONT(options) )
      get_font(DTfscale[(int) display->OLscale],  display->OLscale, DTFONT) ;

  set_min_frame_size() ;
  if (olddate  != options->date)  adjust_date_size() ;
  if (oldtzval != options->tzval) adjust_timezone_size() ;

  set_frame_hints() ;
  set_clock_props() ;
  options->doingprops = FALSE ;
}


/*ARGSUSED*/
void
clock_defaults(item, event)
     Panel_item item ;
     Event *event ;
{
  clock_apply(item, event) ;
  write_resources() ;
}


void
clock_exec(av)       /* Exec a command. */
char *av[] ;
{
  int pid ;

  switch (pid = fork())
    {
      case -1 : perror(DGET("fork")) ;
                return ;
      case  0 : EXECVP(*av, av) ;  /* execvp doesn't return unless failed. */
                perror(DGET("execvp")) ;
                _exit(1) ;
      default : NOTIFY_SET_WAIT3_FUNC(Clock_frame->canvas,
                                      notify_default_wait3, pid) ;
    }
}


/*ARGSUSED*/
void
clock_repaint_proc(canvas, pw, dpy, xid, xrects)
     Canvas canvas ;             /* Canvas handle */
     Xv_Window pw ;              /* Pixwin/Paint window */
     Display *dpy ;              /* Server connection */
     Xv_Window xid ;             /* X11 window handle */
     Xv_xrectlist *xrects ;      /* Damage rectlist */
{
    int minheight, minwidth ;
    XEvent ev ;

    if (!display->started) return ;
    X->repaint = TRUE ;		/* Ready to handle size hints now. */



    /*  The window manager does not read the NormalHints property until the
     *  frame comes out of it's withdrawn state. So we set the initial minimal
     *  size hints here, the very first time through.
     */
 
    if (X->firstpaint == TRUE)
	{
	    X->firstpaint = FALSE ;
	    set_frame_hints() ;
	}

    XSync(X->dpy, 0) ;

    while (XPending(X->dpy) && (XPeekEvent(X->dpy, &ev),
				(ev.type == Expose && ev.xany.window == xid)))
	XNextEvent(X->dpy, &ev) ;

    /*  XXX: Another bloody kludge. If the clock is started from the command
     *       line, it is possible it won't get a WIN_RESIZE event. Because of
     *       this, the canvas dimensions aren't correctly initialised, it which
     *       case we should do it here.
     */

    if (!display->resized)
	{
	    int i = INDEX_FROM_FONT_TYPE(DTFONT);
	    clk->width  = (int) xv_get(canvas, XV_WIDTH) ;
	    clk->height = (int) xv_get(canvas, XV_HEIGHT) ;
	    clk->cheight = clk->height ;
	    minheight = ((options->face == ANALOG) ? MIN_ANALOG_HEIGHT
			 : MIN_DIG_HEIGHT) +
			     display->fheight[i] + GAP +
				 (DATE_ON(options) ? display->fheight[i] + GAP  
				  : 0) ;
	    minwidth  = ((options->face == ANALOG)
			 ? 64 : 6 * (MIN_FONT_WIDTH + MIN_FONT_SPACE)) ;
	    if (clk->width  < minwidth)  clk->width  = minwidth ;       
	    if (clk->height < minheight) clk->height = minheight ;      

	    if (DATE_ON(options))
		clk->cheight -= (display->fheight[i] + GAP) ;
	    if (options->tzval == TRUE && IS_TIMEZONE(options))
		clk->cheight -= (display->fheight[i] + GAP) ;
	}

    if ((int) xv_get(Clock_frame->frame, FRAME_CLOSED) == FALSE)
	{
	    options->face = options->savedFace ;
	    handle_repaint(options->face) ;
	}
}


/*ARGSUSED*/
void
clock_reset(item, event)
     Panel_item item ;
     Event *event ;
{
  set_option_values() ;   /* Set property items from option values. */
}


/*ARGSUSED*/
void
clock_resize_proc(canvas, width, height)
     Canvas canvas ;
     int width, height ;
{
  int i ;

  if (!display->started) 
      return ;

  clk->width  = width ;
  clk->height = height ;
  clk->cheight = height ;
  display->resized = 1 ;

  display->OLscale = S_SMALL ;
  for (i = 0; i < MAXSCALES; i++)
    if (height > scaleH[i])
	display->OLscale = (enum scale_type) i ;

  if ( USE_SFONT(options) )
      get_font(Sfscale[(int) display->OLscale],  display->OLscale, SFONT) ;
  if ( USE_DTFONT(options) )
      get_font(DTfscale[(int) display->OLscale], display->OLscale, DTFONT) ;

  i = INDEX_FROM_FONT_TYPE(DTFONT);
  if (DATE_ON(options))
    clk->cheight -= (display->fheight[i] + GAP) ;
  if (options->tzval == TRUE && IS_TIMEZONE(options))
    clk->cheight -= (display->fheight[i] + GAP) ;

  handle_resize() ;
}


static void
display_prop_sheet()
{
  init_options() ;
  if (xv_get(Clock_props->props, XV_SHOW) == FALSE)
    {
      XV_SET(Clock_props->props, FRAME_PROPS_PUSHPIN_IN, TRUE, 0) ;
      ds_position_popup(Clock_frame->frame, Clock_props->props, DS_POPUP_LOR) ;
      XV_SET(Clock_props->props, XV_SHOW, TRUE, 0) ;
    }
}


static void
draw_circle(svtype, r)
enum sv_type svtype ;
int r ;
{
  XFillArc(X->dpy, X->xid[(int) svtype], X->svgc,
           0, 0, 2 * r, 2 * r, 0, 360 * 64) ;
}


Notify_value
frame_interpose(frame, event, arg, type)
     Frame frame ;
     Event *event ;
     Notify_arg arg ;
     Notify_event_type type ;
{
  int action ;
  Notify_value rc ;

  rc = notify_next_event_func(frame, (Notify_event) event, arg, type) ;
  action = event_action(event) ;

  /* Don't start painting until really mapped */
  if ( action == WIN_VISIBILITY_NOTIFY )
      display->started = TRUE;

  if (action == WIN_RESIZE) 
      set_min_frame_size() ;
  else if (action == ACTION_CLOSE && display->started)
    {
      options->savedFace = options->face ;
      options->face = ICON_ANALOG ;
      display->centerX = 0 ;
      display->centerY = 0 ;
      handle_timer() ;
      handle_repaint(options->face) ;
    }
  return(rc) ;
}


static Menu
gen_tzmenu(path)    /* Generate pull-right menu of available timezones. */
char *path ;
{
  char *fname ;               /* Pointer to current filename. */
  char *pname ;               /* Pointer to current pathname. */
  DIR *dirp ;                 /* Stream for monitoring directory. */
  struct dirent *filep ;      /* Pointer to current directory entry. */
  struct stat cstat ;         /* For information on current file. */
  Menu menu, pullright ;
  Menu_item item ;

  menu = (Menu) xv_create(XV_NULL, MENU, 0) ;
  if ((dirp = opendir(path)) == NULL)
    {
      FPRINTF(stderr, mess[(int) M_TZINFO]) ;
      return(menu) ;
    }
  while ((filep = readdir(dirp)) != NULL)
    {
#ifdef SVR4
      if (filep->d_ino == 0) continue ;
#else
      if (filep->d_fileno == 0) continue ;
#endif /*SVR4*/
      if (!DOTS(filep->d_name))  /* Is it the . or .. entry? */
        {
          fname = malloc((unsigned) (strlen(filep->d_name)+1)) ;
          STRCPY(fname, filep->d_name) ;
          pname = malloc((unsigned) (strlen(path) + strlen(filep->d_name)+2)) ;
          SPRINTF(pname, "%s/%s", path, filep->d_name) ;
          STAT(pname, &cstat) ;
          if ((cstat.st_mode & S_IFMT) == DIRECTORY)      /* Directory? */
            {
              pullright = gen_tzmenu(pname) ;
              item = xv_create(XV_NULL,            MENUITEM,
                               MENU_CLIENT_DATA,   pname,
                               MENU_STRING,        fname,
                               MENU_PULLRIGHT,     pullright,
                               0) ;
            }
          else if ((cstat.st_mode & S_IFMT) == REGULAR)   /* Ordinary file? */
            item = xv_create(XV_NULL,          MENUITEM,
                             MENU_CLIENT_DATA, pname,
                             MENU_ACTION_ITEM, fname, menu_choose,
                             0) ;

          XV_SET(menu, MENU_APPEND_ITEM, item, 0) ;
        }
    }    
  CLOSEDIR(dirp) ;
  return(menu) ;
}


void
get_font(scale, olscale, ftype)
     int scale;
     enum scale_type olscale;
     enum font_type ftype;
{
    Xv_font pf;
    XFontSetExtents *extents;
    XFontSet fontset;
    int i = INDEX_FROM_FONT_TYPE(ftype);

    /* already found this font */
    if ( X->font[i] )
	return;

    pf = xv_find(Clock_frame->frame, FONT,
		 FONT_FAMILY,	FONT_FAMILY_SANS_SERIF,
		 FONT_SCALE,	scale,
		 NULL);


    /* hmmp, seems kind of sad that clock can't run at all without
     * a particular text font...
     */
    if ( !pf ) {
	FPRINTF(stderr, mess[(int) M_FONT], display->progname) ;
	exit(1) ;
    }

    X->font[i] = pf;
    fontset = (XFontSet) xv_get(pf, FONT_SET_ID);
    extents = XExtentsOfFontSet( fontset ); 		/* do not free()! */

    display->fheight[i] = extents->max_logical_extent.height;
    display->fascent[i] = -(extents->max_logical_extent.y);
}


void
get_frame_size(x, y, w, h)
int *x, *y, *w, *h ;
{
  DS_GET_FRAME_SIZE(Clock_frame->frame, x, y, w, h) ;
}


char *
get_resource(rtype)      /* Get clock resource from server. */
enum res_type rtype ;
{
  char str[MAXSTRING] ;

  STRCPY(str, clk_res[(int) rtype]) ;
  return(ds_get_resource(X->rDB, options->appname, str)) ;
}


int
get_str_width(ftype, buf)
     enum font_type ftype ;
     char *buf ;
{
    int i = INDEX_FROM_FONT_TYPE(ftype);
    XFontSet fontset = (XFontSet) xv_get(X->font[i], FONT_SET_ID);
    return XmbTextEscapement(fontset, buf, strlen(buf));
}


void
grow_font(factor)
int factor ;
{
  int i, j ;

  for (i = 0; i < 14; i++)
    for (j = 0; j < 4;  j++)
      {
        X->workingPoints[i][j].x = (defaultPoints[i][j].x * factor) ;
        X->workingPoints[i][j].y = (defaultPoints[i][j].y * factor) ;
      }
  for (i = 14; i < 15; i++)
    for (j = 0; j < 6; j++)
      {
        X->workingPoints[i][j].x = (defaultPoints[i][j].x * factor) ;
        X->workingPoints[i][j].y = (defaultPoints[i][j].y * factor) ;
      }
}


static Notify_value
icon_interpose(icon, event, arg, type)
     Icon icon ;
     Event *event ;
     Notify_arg arg ;
     Notify_event_type type ;
{
  Notify_value rc ;
 
  rc = notify_next_event_func(icon, (Notify_event) event, arg, type) ;
  if ((event_action(event) == WIN_REPAINT) &&
      ((int) xv_get(Clock_frame->frame, FRAME_CLOSED) == TRUE))
    {
      options->face = ICON_ANALOG ;
      display->centerX = 0 ;
      display->centerY = 0 ;
      handle_repaint(options->face) ;
    }
  return(rc) ;
}


void
init_analog(w, h)       /* Initialise ANALOG display. */
int w, h ;
{
  if (w == X->latest_analog_w &&      /* Don't redo if already done. */
      h == X->latest_analog_h)
    return ;

  make_image(HANDSPR, w, h) ;
  display->hands_height = h;
  display->hands_width  = w;

  make_image(DOTSPR, w, h) ;

  make_image(SPOTPR, w / 12, h / 12) ;
  draw_circle(SPOTPR, armwidth(w) / 8) ;
  paint_ticks(DOTSPR, w / 2, SPOTPR) ;

  /* done with spot */
  XFreePixmap(X->dpy, X->xid[(int) SPOTPR]);
  X->xid[SPOTPR] = NULL;

  X->latest_analog_w = w ;      /* Remember what did. */
  X->latest_analog_h = h ;
}


void
init_gray_patch()
{
    XGCValues gc_val;
    unsigned long gc_mask;

    X->xid[(int) GRAY_PATCH] 
	= XCreateBitmapFromData(X->dpy, RootWindow(X->dpy, DefaultScreen(X->dpy)),
				gray_bits, 16, 16);

    /* We need graphics contexts for working with the 1 bit deep server images. */

    gc_mask = GCForeground | GCBackground ;
    gc_val.foreground = 1 ;
    gc_val.background = 0 ;
    X->svgc = XCreateGC(X->dpy, X->xid[(int) GRAY_PATCH],
			gc_mask, &gc_val) ;
 
    gc_mask |= GCFunction;
    gc_val.function = GXxor;
    X->svlinegc = XCreateGC(X->dpy, X->xid[(int) GRAY_PATCH],
			    gc_mask, &gc_val) ;

    gc_mask = GCForeground | GCBackground | GCFillStyle;
    gc_val.fill_style = FillOpaqueStippled;
    X->svropgc = XCreateGC(X->dpy, X->xid[(int) GRAY_PATCH],
			   gc_mask, &gc_val) ;

    X->svtilegc = XCreateGC(X->dpy, X->xid[(int) GRAY_PATCH],
			    gc_mask, &gc_val) ;
}


void
init_icon_images()
{
    
    if (X->xid[(int) ICONANALOG]) return ; /* Don't do multiple times. */

    make_image(ICONHANDSPR, 64, 64) ;

    if (options->iconpath != NULL)
	{
	    Server_image iconface ;
	    Server_image romface ;
	    char errmes[MAXSTRING] ;


	    /* BUG:  icon_load_svrim only works for SunView1 icon's! */
	    iconface = (Server_image)
		icon_load_svrim(options->iconpath, errmes) ;
	    romface  = (Server_image)
		icon_load_svrim(options->iconpath, errmes) ;
	    if (iconface == NULL || romface == NULL)
		{
		    FPRINTF(stderr, "%s", errmes) ;
		    xv_usage(vstrs[(int) V_LABEL]) ;
		}
	    X->xid[(int) ICONANALOG] = (Drawable) xv_get(iconface, XV_XID) ;
	    X->xid[(int) ICONROMAN]  = (Drawable) xv_get(romface, XV_XID) ;
	    /* forget about Server_image handles... */
	}
    else
	{
	    X->xid[(int) ICONANALOG] 
		= XCreateBitmapFromData(X->dpy, RootWindow(X->dpy, DefaultScreen(X->dpy)),
					clock_xbm_bits, 64, 64);
	    X->xid[(int) ICONROMAN]  
		= XCreateBitmapFromData(X->dpy, RootWindow(X->dpy, DefaultScreen(X->dpy)),
					clock_rom_xbm_bits, 64, 64);
	}
}


void
init_icon()
{
    unsigned long gc_mask;
    XGCValues gc_val;
    Rect ir, lr ;
    Icon icon;

    icon = (Icon) xv_create(XV_NULL,      ICON,
			    WIN_RETAINED, TRUE,
			    0) ;
    NOTIFY_INTERPOSE_EVENT_FUNC(icon, icon_interpose, 0) ;
    if (options->iconlabel)
	{
	    rect_construct(&ir, 0, 0, 64, 64) ;
	    rect_construct(&lr, 0, 67, 64, 20) ;
	    XV_SET(icon,
		   XV_HEIGHT,        84,
		   ICON_TRANSPARENT, TRUE,
		   ICON_IMAGE_RECT,  &ir,
		   ICON_LABEL_RECT,  &lr,
		   XV_LABEL,         options->iconlabel,
		   0) ;
	}
    XV_SET(Clock_frame->frame, FRAME_ICON, icon, 0) ;

    X->xid[(int) CLOCK_ICON] = (Drawable) xv_get(icon, XV_XID) ;

    gc_mask = GCForeground | GCBackground | GCGraphicsExposures ;
    gc_val.foreground = X->foregnd ;
    gc_val.background = X->wscolor ;
    gc_val.graphics_exposures = False ;
    X->iconsolidgc = XCreateGC(X->dpy, X->xid[(int) CLOCK_ICON],
			       gc_mask, &gc_val) ;
  
    gc_mask = GCForeground | GCBackground |
	GCFunction   | GCGraphicsExposures ;
    gc_val.foreground = X->foregnd ^ X->wscolor ;
    gc_val.background = X->wscolor ;
    gc_val.function   = GXxor ;
    gc_val.graphics_exposures = False ;
    X->iconxorgc = XCreateGC(X->dpy, X->xid[(int) CLOCK_ICON],
			     gc_mask, &gc_val) ;
}


/* FRAME_DONE_PROC on props frame */
static void
props_done_proc()
{
    /*
     * Since props sheet is not likely to be called up frequenly
     * during the lifetime of 'clock', don't bother to keep it
     * around...
     */
    xv_destroy( Clock_props->props );
    free( Clock_props );
    Clock_props = NULL;
}



void
init_options()
{
  int gap, maxw ;
  int islocal ;      /* Set if timezone is set to "local". */
  Menu tzmenu;

  if (Clock_props != NULL) return ;     /* Just return if already created. */

  Clock_props = clock_props_objects_initialize(NULL, Clock_frame->frame) ;
  XV_SET(Clock_props->props,
         XV_HELP_DATA,      hstr[(int) H_PFRAME],
	 FRAME_DONE_PROC,   props_done_proc,
         0) ;

  ds_justify_items(Clock_props->panel, FALSE);

  DS_ADD_HELP(Clock_props->panel,           (char *)hstr[(int) H_PPANEL]) ;
  DS_ADD_HELP(Clock_props->faceChoice,      (char *)hstr[(int) H_FCHOICE]) ;
  DS_ADD_HELP(Clock_props->iconChoice,      (char *)hstr[(int) H_ICHOICE]) ;
  DS_ADD_HELP(Clock_props->digChoice,       (char *)hstr[(int) H_DIGSTYLE]) ;
  DS_ADD_HELP(Clock_props->secondsToggle,   (char *)hstr[(int) H_DISPSTYLE]) ;
  DS_ADD_HELP(Clock_props->dateToggle,      (char *)hstr[(int) H_DISPSTYLE]) ;
  DS_ADD_HELP(Clock_props->timezone,        (char *)hstr[(int) H_TZSTYLE]) ;
  DS_ADD_HELP(Clock_props->stopwatch,       (char *)hstr[(int) H_SCHOICE]) ;
  DS_ADD_HELP(Clock_props->alarm,           (char *)hstr[(int) H_ACHOICE]) ;
  DS_ADD_HELP(Clock_props->acommand,        (char *)hstr[(int) H_ACMD]) ;
  DS_ADD_HELP(Clock_props->repeat,          (char *)hstr[(int) H_REPEAT]) ;
  DS_ADD_HELP(Clock_props->hcommand,        (char *)hstr[(int) H_HCMD]) ;
  DS_ADD_HELP(Clock_props->apply_button,    (char *)hstr[(int) H_APPLY]) ;
  DS_ADD_HELP(Clock_props->tzbut,           (char *)hstr[(int) H_TZBUT]) ;
  DS_ADD_HELP(Clock_props->tzname,          (char *)hstr[(int) H_TZNAME]) ;
  DS_ADD_HELP(Clock_props->hrval,           (char *)hstr[(int) H_HRCHOICE]) ;
  DS_ADD_HELP(Clock_props->minval,          (char *)hstr[(int) H_MINCHOICE]) ;
  DS_ADD_HELP(Clock_props->defaults_button, (char *)hstr[(int) H_DEF]) ;
  DS_ADD_HELP(Clock_props->reset_button,    (char *)hstr[(int) H_RESET]) ;

  XV_SET(Clock_props->faceChoice,    PANEL_VALUE, options->face, 0) ;
  XV_SET(Clock_props->iconChoice,    PANEL_VALUE, options->icontype, 0) ;
  XV_SET(Clock_props->digChoice,     PANEL_VALUE, options->digtype, 0) ; 
  XV_SET(Clock_props->secondsToggle, PANEL_VALUE, options->seconds, 0) ; 
  XV_SET(Clock_props->dateToggle,    PANEL_VALUE, options->date, 0) ;
  XV_SET(Clock_props->timezone,      PANEL_VALUE, options->tzval, 0) ;    
  XV_SET(Clock_props->stopwatch,     PANEL_VALUE, (int) options->schoice, 0) ;
  XV_SET(Clock_props->acommand,      PANEL_VALUE, options->acmd, 0) ;  
  XV_SET(Clock_props->repeat,        PANEL_VALUE, (int) options->achoice, 0) ; 
  XV_SET(Clock_props->hcommand,      PANEL_VALUE, options->hcmd, 0) ;   
  XV_SET(Clock_props->tzname,        PANEL_VALUE, options->tzname, 0) ;    
  XV_SET(Clock_props->hrval,         PANEL_VALUE, options->ahrval, 0) ;
  XV_SET(Clock_props->minval,        PANEL_VALUE, options->aminval, 0) ;    

  tzmenu = gen_tzmenu(TZDIR) ;
  islocal    = ((int) xv_get(Clock_props->timezone, PANEL_VALUE) == 0) ;
  XV_SET(Clock_props->tzbut,
         XV_SHOW,         !islocal,
         PANEL_ITEM_MENU, tzmenu,
         0) ;
  XV_SET(Clock_props->tzname, PANEL_INACTIVE, islocal, 0) ;

  gap = (int) xv_get(Clock_props->panel, PANEL_ITEM_X_GAP) ;
  XV_SET(Clock_props->dateToggle,
         XV_X, (int) xv_get(Clock_props->secondsToggle, XV_X) +
               (int) xv_get(Clock_props->secondsToggle, XV_WIDTH) + gap,
         XV_Y, (int) xv_get(Clock_props->secondsToggle, XV_Y),
         0) ;
  XV_SET(Clock_props->tzbut,
         XV_X, (int) xv_get(Clock_props->timezone, XV_X) +
               (int) xv_get(Clock_props->timezone, XV_WIDTH) + gap,
         XV_Y, (int) xv_get(Clock_props->timezone, XV_Y),
         0) ;
  XV_SET(Clock_props->tzname,
         XV_X, (int) xv_get(Clock_props->tzbut, XV_X) +
               (int) xv_get(Clock_props->tzbut,    XV_WIDTH) + gap,
         XV_Y, (int) xv_get(Clock_props->tzbut, XV_Y) + 5,
         0) ;
  XV_SET(Clock_props->hrval,
         XV_X, (int) xv_get(Clock_props->alarm, XV_X) +
               (int) xv_get(Clock_props->alarm, XV_WIDTH) + gap,
         XV_Y, (int) xv_get(Clock_props->alarm, XV_Y),
         0) ;
  XV_SET(Clock_props->minval,
         XV_X, (int) xv_get(Clock_props->alarm, XV_X) +
               (int) xv_get(Clock_props->alarm, XV_WIDTH) +
               (int) xv_get(Clock_props->hrval, XV_WIDTH) + (2 * gap),
         XV_Y, (int) xv_get(Clock_props->alarm, XV_Y),
         0) ;

  ds_center_items(Clock_props->panel, -1,
                  Clock_props->apply_button,
                  Clock_props->defaults_button,
                  Clock_props->reset_button,
                  0) ;

/*  Determine what is the maximum width of the property panel, and set it's
 *  width accordingly. Ugly code, but it seems to work.
 */

  maxw = 0 ;
  test_prop_width(Clock_props->faceChoice,   &maxw) ;
  test_prop_width(Clock_props->iconChoice,   &maxw) ;
  test_prop_width(Clock_props->digChoice,    &maxw) ;
  test_prop_width(Clock_props->stopwatch,    &maxw) ;
  test_prop_width(Clock_props->acommand,     &maxw) ;
  test_prop_width(Clock_props->repeat,       &maxw) ;
  test_prop_width(Clock_props->hcommand,     &maxw) ;
  test_prop_width(Clock_props->dateToggle,   &maxw) ;
  test_prop_width(Clock_props->tzname,       &maxw) ;
  test_prop_width(Clock_props->minval,       &maxw) ;
  test_prop_width(Clock_props->reset_button, &maxw) ;
  XV_SET(Clock_props->props, XV_WIDTH, maxw+10, 0) ;
}


void
init_points()
{
    X->workingPoints[0] = X->dummyA;
    X->workingPoints[1] = X->dummyB;
    X->workingPoints[2] = X->dummyC;
    X->workingPoints[3] = X->dummyD;
    X->workingPoints[4] = X->dummyE;
    X->workingPoints[5] = X->dummyF;
    X->workingPoints[6] = X->dummyG;
    X->workingPoints[7] = X->dummyH;
    X->workingPoints[8] = X->dummyI;
    X->workingPoints[9] = X->dummyJ;
    X->workingPoints[10] = X->dummyK;
    X->workingPoints[11] = X->dummyL;
    X->workingPoints[12] = X->dummyM;
    X->workingPoints[13] = X->dummyN;
    X->workingPoints[14] = X->dummyO;
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
    Canvas_paint_window pw;

    XV_SET(Clock_frame->canvas,
	   CANVAS_AUTO_EXPAND,  TRUE,
	   CANVAS_AUTO_SHRINK,  TRUE,
	   CANVAS_AUTO_CLEAR,   TRUE,
	   CANVAS_RETAINED,     FALSE,
	   XV_HELP_DATA,        hstr[(int) H_CANVAS],
	   0) ;

    pw = canvas_paint_window(Clock_frame->canvas) ;
    X->xid[(int) CLOCK_WIN]  = (Drawable) xv_get(pw,   XV_XID) ;
    XV_SET(pw, WIN_CONSUME_X_EVENT_MASK, VisibilityChangeMask, 0) ;
}


void
make_frame()
{
  extern char *ds_relname(), *ds_hostname() ;
  Colormap def_cmap ;
  char *str_type[20], *tool_label ;
  XColor ccol ;
  XrmValue value ;
  Cms cms;

/* Set label on frame. We couldn't do this before we got the display. */

  if (options->titleline == NULL)
    {
      tool_label = malloc(strlen(vstrs[(int) V_LABEL]) +
                          strlen(ds_relname()) +
                          strlen(ds_hostname(X->dpy)) + 3) ;

      SPRINTF(tool_label, "%s %s%s", vstrs[(int) V_LABEL],
              ds_relname(), ds_hostname(X->dpy)) ;
    }
  else tool_label = options->titleline ;

  XV_SET(Clock_frame->frame,
         FRAME_NO_CONFIRM,      TRUE,
         FRAME_SHOW_HEADER,     TRUE,
         FRAME_SHOW_LABEL,      options->title,
         FRAME_LABEL,           tool_label,
         FRAME_PROPERTIES_PROC, show_props,
         XV_HELP_DATA,          hstr[(int) H_FRAME],
         0) ;

  X->opvals[(int) GOR]  = GXor ;
  X->opvals[(int) GSRC] = GXcopy ;
  X->opvals[(int) GSET] = GXset ;
  X->opvals[(int) GXOR] = GXxor ;
 
  X->depth   = (int) xv_get(Clock_frame->frame, XV_DEPTH) ;
  cms     = (Cms) xv_get(Clock_frame->frame, WIN_CMS) ;
  X->foregnd = xv_get(cms, CMS_FOREGROUND_PIXEL) ;
  X->backgnd = xv_get(cms, CMS_BACKGROUND_PIXEL) ;

/*  XXX: Kludge to get the clock working correctly on monochrome NCD screens.
 *       Surely, there has got to be a better way of doing this?
 */

  if (X->depth == 1 && BlackPixel(X->dpy, DefaultScreen(X->dpy)) == 0)
    X->opvals[(int) GXOR] = GXequiv ;

  def_cmap   = (Colormap) xv_get(cms, CMS_CMAP_ID) ;
  X->wscolor = X->backgnd ;      /* Default in case of error. */

  if (options->use_wincol == FALSE)
    {
      if (XrmGetResource(X->rDB, WORKSPACE_RES, WORKSPACE_CLASS_RES,
                         str_type, &value) == NULL)
        return ;
 
      if (XParseColor(X->dpy, def_cmap, value.addr, &ccol) != 0)
        if (XAllocColor(X->dpy, def_cmap, &ccol) == True)
          X->wscolor = ccol.pixel ;
    }
  else X->wscolor = X->backgnd ;

/*  If the screen depth is 1, and the workspace color is the same as 
 *  the foreground color, then the background color for the icon is
 *  automatically set to the background color of the window.
 */

  if (X->depth == 1 && (X->foregnd == X->wscolor))
    {
      X->wscolor         = X->backgnd ;
      options->use_wincol = TRUE ;
    }
}


void
make_gcs()
{
    Drawable xid ;
    XGCValues gc_val;
    unsigned long gc_mask;

    xid = (Drawable) xv_get(Clock_frame->frame, XV_XID) ;

    gc_mask = GCForeground | GCBackground | GCGraphicsExposures ;
    gc_val.foreground = X->foregnd ;
    gc_val.background = X->backgnd ;
    gc_val.graphics_exposures = False ;
    X->gc = XCreateGC(X->dpy, xid, gc_mask, &gc_val) ;

    X->linegc = XCreateGC(X->dpy, xid, gc_mask, &gc_val) ;
    if (X->depth > 1)
	XSetForeground(X->dpy, X->linegc, X->foregnd ^ X->backgnd) ;
    XSetFunction(X->dpy, X->linegc, GXxor) ;

    X->ropgc = XCreateGC(X->dpy, xid, gc_mask, &gc_val) ;
    XSetFillStyle(X->dpy, X->ropgc, FillOpaqueStippled) ;
}


void
make_graphics(argc, argv)       /* Allocate memory for graphics objects. */
int *argc ;
char **argv ;
{
  X->dpy        = (Display *) xv_get(Clock_frame->frame, XV_DISPLAY) ;
  X->firstpaint = TRUE ;
}


static void
make_image(svtype, w, h)
     enum sv_type svtype ;
     int w, h ;
{
    if ( X->xid[(int) svtype] )
	XFreePixmap(X->dpy, X->xid[(int) svtype]);

  X->xid[(int) svtype] 
      = XCreatePixmap(X->dpy, RootWindow(X->dpy, DefaultScreen(X->dpy)),
		      w, h, 1/* depth */);

  xclear(CLOCK_IMAGE, svtype, 0, 0, w, h) ;
}


static Menu
make_menu()
{
  Menu_item mi ;

  if (X->menu) return(X->menu) ; /* Return if menu has already been created. */

  X->menu = clock_menu_create((caddr_t) INSTANCE, (Xv_opaque) NULL) ;
  mi = (Menu_item) xv_get(X->menu, MENU_NTH_ITEM, MENU_PROPS) ;
  XV_SET(mi, XV_HELP_DATA, hstr[(int) H_PROPS], 0) ;
  return(X->menu) ;
}


/*ARGSUSED*/
static void
menu_choose(menu, item)   /* Set timezone name for timezone menu selection. */
Menu menu ;
Menu_item item ;
{  
  char *data, fstr[MAXSTRING], tz[MAXSTRING] ;
   
  SPRINTF(fstr, "%s/%%s", TZDIR) ;
  data = (char *) xv_get(item, MENU_CLIENT_DATA, 0) ;
  SSCANF(data, fstr, tz) ;
  XV_SET(Clock_props->tzname, PANEL_VALUE, tz, 0) ;
}


void
paint_hand(dtype, svtype, x1, y1, x2, y2, x3, y3, angle, diameter)
     enum drawing_type dtype ;
     enum sv_type svtype ;
     int x1, y1, x2, y2, x3, y3, angle, diameter ;
{
    XPoint vlist[3] ;
    GC gc ;
    register int radius = diameter / 2 ;
    XGCValues gc_val;
    unsigned long gc_mask;

    vlist[0].x = rotx(x1, y1, radius, angle) ; /* Rotate */
    vlist[0].y = roty(x1, y1, radius, angle) ;
    vlist[1].x = rotx(x2, y2, radius, angle) ;
    vlist[1].y = roty(x2, y2, radius, angle) ;
    vlist[2].x = rotx(x3, y3, radius, angle) ;
    vlist[2].y = roty(x3, y3, radius, angle) ;


    gc = (dtype == SURFACE) ? X->iconsolidgc : X->svtilegc;

    gc_mask 
	= GCTileStipXOrigin | GCTileStipYOrigin | GCStipple | GCFunction | GCFillStyle ;
    gc_val.stipple = X->xid[(int) GRAY_PATCH] ;
    gc_val.ts_x_origin = 0 ;
    gc_val.ts_y_origin = 0 ;
    gc_val.function = GXcopy ;
    gc_val.fill_style = FillOpaqueStippled ;
    XChangeGC(X->dpy, gc, gc_mask, &gc_val) ;

    XFillPolygon(X->dpy, X->xid[(int) svtype], gc, &vlist[0], 3,
		 Convex, CoordModeOrigin) ;

    xline(dtype, svtype, GSRC,
	  vlist[0].x, vlist[0].y, vlist[1].x, vlist[1].y) ;
    xline(dtype, svtype, GSRC,
	  vlist[0].x, vlist[0].y, vlist[2].x, vlist[2].y) ;
    xline(dtype, svtype, GSRC,
	  vlist[1].x, vlist[1].y, vlist[2].x, vlist[2].y) ;
}


void
put_resource(rtype, value)   /* Put clock resource into deskset database. */
enum res_type rtype ;
char *value ;
{
  ds_put_resource(&X->desksetDB, options->appname,
                  (char *)clk_res[(int) rtype], value) ;
}


void
save_cmdline(argc, argv)
int argc ;
char *argv[] ;
{
  DS_SAVE_CMDLINE(Clock_frame->frame, argc, argv) ; 
}


void
save_resources()
{
  ds_save_resources(X->desksetDB) ;
}


void
set_def_dims()
{
    int ht, minheight, minwidth, twd, wd ;
    int i = INDEX_FROM_FONT_TYPE(DTFONT);

    if (display->clock_usersetsize == TRUE)
	if (display->iwidth != -1 && display->iheight != -1)
	    {
		minheight = ((options->face == ANALOG) ? MIN_ANALOG_HEIGHT
			     : MIN_DIG_HEIGHT) +
				 display->fheight[i] + GAP +
				     (DATE_ON(options) ? display->fheight[i] + GAP
				      : 0) ;
		minwidth  = ((options->face == ANALOG)
			     ? 64 : 6 * (MIN_FONT_WIDTH + MIN_FONT_SPACE)) ;
		if (display->iwidth  < minwidth)  display->iwidth  = minwidth ;
		if (display->iheight < minheight) display->iheight = minheight ;
		clk->width = display->iwidth ;
		clk->cheight = clk->height = display->iheight ;
		return ;
	    }

    /*  Set the default dimension for the clock based on the current scale.
     *  If the scale is SMALL, then the minimum dimensions are used; if the
     *  scale is medium (true when no scale given), then the default dimensions
     *  are used. Otherwise, dimensions are taken from an array of values,
     *  dependant upon the current scale.
     */

    if (display->OLscale == S_SMALL)
	ht = DIGITAL_ON(options) ? MIN_DIG_HEIGHT : MIN_ANALOG_HEIGHT ;
    else if (display->OLscale == S_MEDIUM)
	ht = DIGITAL_ON(options) ? DEF_DIG_HEIGHT : DEF_ANALOG_HEIGHT ;
    else ht = scaleH[(int) display->OLscale] ;

    if (DATE_ON(options))
	ht += display->fheight[i] + GAP ;
    if (options->tzval == TRUE && IS_TIMEZONE(options))
	ht += display->fheight[i] + GAP ;
    clk->height = clk->cheight = ht ;
    XV_SET(Clock_frame->canvas, XV_HEIGHT, ht, 0) ;

    if (display->OLscale == S_SMALL)
	wd = DIGITAL_ON(options) ? MIN_DIG_WIDTH : MIN_ANALOG_WIDTH ;
    else if (display->OLscale == S_MEDIUM)
	wd = DIGITAL_ON(options) ? DEF_DIG_WIDTH : DEF_ANALOG_WIDTH ;
    else wd = scaleH[(int) display->OLscale] ;

    if (options->tzval == TRUE && IS_TIMEZONE(options))
	{
	    twd = get_str_width(DTFONT, options->tzname) ;
	    twd += (2 * GAP) ;
	    if (wd < twd) wd = twd ;
	}
    clk->width = wd ;
    XV_SET(Clock_frame->canvas, XV_WIDTH, wd, 0) ;
    window_fit(Clock_frame->frame);
}


void
set_frame_hints()     /* Setup minimum and maximum frame size hints. */
{
  int min_width;
  int min_height;
  int i = INDEX_FROM_FONT_TYPE(DTFONT);

  min_height = ((options->face == ANALOG) ? MIN_ANALOG_HEIGHT
                                            : MIN_DIG_HEIGHT) +
                 display->fheight[i] + GAP +
                 (DATE_ON(options) ? display->fheight[i] + GAP
                                   : 0) ;
  min_width  = ((options->face == ANALOG)
                 ? 64 : 6 * (MIN_FONT_WIDTH + MIN_FONT_SPACE)) ;
 
  if (options->tzval == TRUE && IS_TIMEZONE(options))
    {
      int twd ;           /* Width in pixels of the timezone name. */
      twd = get_str_width(DTFONT, options->tzname) ;
      twd += (2 * GAP) ;
      if (min_width < twd) min_width = twd ;
    }

  xv_set(Clock_frame->frame, FRAME_MIN_SIZE, min_width, min_height, NULL);
}


void
set_frame_size(x, y, w, h)
int x, y, w, h ;
{
  DS_SET_FRAME_SIZE(Clock_frame->frame, x, y, w, h) ;
}


void
set_option_values()       /* Set property items from option values. */
{

  if (!Clock_props) {
      return;
  }
  XV_SET(Clock_props->faceChoice,    PANEL_VALUE, (int) options->face,     0) ;
  XV_SET(Clock_props->iconChoice,    PANEL_VALUE, (int) options->icontype, 0) ;
  XV_SET(Clock_props->digChoice,     PANEL_VALUE, (int) options->digtype,  0) ;
  XV_SET(Clock_props->secondsToggle, PANEL_VALUE,       options->seconds,  0) ;
  XV_SET(Clock_props->dateToggle,    PANEL_VALUE,       options->date,     0) ;
  XV_SET(Clock_props->hrval,         PANEL_VALUE,       options->ahrval,   0) ;
  XV_SET(Clock_props->minval,        PANEL_VALUE,       options->aminval,  0) ;
  XV_SET(Clock_props->repeat,        PANEL_VALUE, (int) options->achoice,  0) ;
  XV_SET(Clock_props->stopwatch,     PANEL_VALUE, (int) options->schoice,  0) ;
  XV_SET(Clock_props->timezone,      PANEL_VALUE,       options->tzval,    0) ;
  XV_SET(Clock_props->tzbut,         XV_SHOW,           options->tzval,    0) ;
  XV_SET(Clock_props->tzname,        PANEL_INACTIVE,   !options->tzval,    0) ;

  /* in the case of string based resources, the default 
     value is NULL if the resource has not been set.  
     Sending NULL to the xview panel package gives it fits, 
     so we mask these down to empty strings. */

  if (options->tzname)
    XV_SET(Clock_props->tzname,        PANEL_VALUE,       options->tzname,   0) ;
  else
    XV_SET(Clock_props->tzname,        PANEL_VALUE,       "",   0) ;

  if (options->acmd)
    XV_SET(Clock_props->acommand,      PANEL_VALUE,       options->acmd,     0) ;
  else
    XV_SET(Clock_props->acommand,      PANEL_VALUE,       "",     0) ;

  if (options->hcmd)
    XV_SET(Clock_props->hcommand,      PANEL_VALUE,       options->hcmd,     0) ;
  else
    XV_SET(Clock_props->hcommand,      PANEL_VALUE,       "",     0) ;
  set_clock_props() ;
}


void
set_stopwatch_val(val)
StopType val ;
{
  XV_SET(Clock_props->stopwatch, PANEL_VALUE, (int) val, 0) ;
}


void
set_timer(vsec, vusec, isec, iusec)
int vsec, vusec, isec, iusec ;
{
  int setfunc = FALSE ;
  struct timeval tv ;

  display->timer.it_value.tv_usec    = vusec ;
  display->timer.it_value.tv_sec     = vsec ;
  display->timer.it_interval.tv_usec = iusec ;
  display->timer.it_interval.tv_sec  = isec ;

/* Utilise notifier hack to avoid stopped clock bug */

  tv = display->timer.it_value ;
  tv.tv_sec *= 2 ;
  NOTIFY_SET_SIGNAL_CHECK(tv) ;
 
  if (vsec > 0 || options->testing == TRUE) setfunc = TRUE ;
  NOTIFY_SET_ITIMER_FUNC(Clock_frame->frame,
                         setfunc ? (Notify_func) timer_expired :
                                    NOTIFY_FUNC_NULL,
                         ITIMER_REAL, &display->timer, ITIMER_NULL) ;
}


void
set_tzval(tzval)
int tzval ;
{
  int islocal ;

  XV_SET(Clock_props->timezone, PANEL_VALUE, tzval, 0) ;
  islocal = (tzval == 0) ;

  options->tzval = (int) xv_get(Clock_props->timezone, PANEL_VALUE) ;

  if ( USE_DTFONT(options) )
      get_font(DTfscale[(int) display->OLscale], display->OLscale, DTFONT) ;

  read_str(&options->tzname,
           (char *) xv_get(Clock_props->tzname, PANEL_VALUE)) ;
  XV_SET(Clock_props->tzbut,  XV_SHOW,        !islocal, 0) ;
  XV_SET(Clock_props->tzname, PANEL_INACTIVE,  islocal, 0) ;

  adjust_timezone_size() ;
}


Menu_item
show_props(item, op)
Menu_item item ;
Menu_generate op ;
{
  if (op == MENU_NOTIFY) display_prop_sheet() ;
  return(item) ;
}


void
start_tool()
{
  xv_main_loop(Clock_frame->frame) ;
}


/*ARGSUSED*/
void
stopwatch_proc(panel_item, value, event)
Panel_item panel_item ;
int value ;
Event *event ;
{
  options->schoice = (StopType) value ;
  do_stopwatch(value) ;
}


static void
test_prop_width(item, maxwidth)
Xv_opaque item ;
int *maxwidth ;
{
  int x = (int) xv_get(item, XV_X) ;
  int w = (int) xv_get(item, XV_WIDTH) ;

  if (x + w > *maxwidth) *maxwidth = x + w ;
}


/*ARGSUSED*/
Notify_value
timer_expired(me, which)
Notify_value me ;
int which ;
{
  handle_time(options->face) ;
  return(NOTIFY_DONE) ;
}


/*ARGSUSED*/
void
timezone_choice_proc(panel_item, value, event)
     Panel_item  panel_item ;
     int value ;
     Event *event ;
{
  int islocal ;

  islocal = ((int) xv_get(Clock_props->timezone, PANEL_VALUE) == 0) ;
  XV_SET(Clock_props->tzbut,  XV_SHOW,        !islocal, 0) ;
  XV_SET(Clock_props->tzname, PANEL_INACTIVE,  islocal, 0) ;
}



void
xclear(dtype, svtype, x, y, width, height)
     enum drawing_type dtype ;
     enum sv_type svtype ;
     int x, y, width, height ;
{
    if (dtype == CLOCK_IMAGE)
	{
	    XSetFunction(X->dpy, X->svgc, GXclear) ;
	    XCopyArea(X->dpy, X->xid[(int) svtype], X->xid[(int) svtype],
		      X->svgc, 0, 0, width, height, 0, 0) ;
	    XSetFunction(X->dpy, X->svgc, GXcopy) ;
	}
    else /* (dtype == SURFACE) */
	XClearArea(X->dpy, X->xid[(int) svtype], x, y, width, height, False) ;
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


void
ximage(dtype, dest, x, y, width, height, op, src)
     enum drawing_type dtype ;
     enum sv_type dest, src ;
     enum op_type op ;
     int x, y, width, height ;
{
    XGCValues gc_val;
    unsigned long gc_mask;
    GC gc ;

    gc_mask = GCTileStipXOrigin | GCTileStipYOrigin |  GCStipple 
	| GCFunction | GCFillStyle ;
    gc_val.stipple = X->xid[(int) src] ;
    gc_val.ts_x_origin = x ;
    gc_val.ts_y_origin = y ;
    gc_val.function = X->opvals[(int) op] ;
    gc_val.fill_style = FillOpaqueStippled ;

    if (dtype == CLOCK_IMAGE) 
	gc = X->svropgc ;
    else if (dest  == CLOCK_WIN)
	gc = X->ropgc ;
    else
	gc = X->iconsolidgc ;

  XChangeGC(X->dpy, gc, gc_mask, &gc_val) ;
  XFillRectangle(X->dpy, X->xid[(int) dest], gc, x, y, width, height) ;
}
 
 
void   
xline(dtype, svtype, op, x1, y1, x2, y2)
     enum drawing_type dtype ;
     enum sv_type svtype ;
     enum op_type op ;
     int x1, y1, x2, y2 ;
{      
    XGCValues gc_val;
    unsigned long gc_mask;
    GC gc ;
       
    if (dtype == CLOCK_IMAGE)
	gc = X->svlinegc ;
    else if (svtype == CLOCK_ICON) {
	if (op == GSRC)
	    gc = X->iconsolidgc ;
	else
	    gc = X->iconxorgc ;   
    }
    else
	gc = X->linegc ;

    gc_mask = GCFunction | GCFillStyle ;
    gc_val.function =  X->opvals[(int) op] ;
    gc_val.fill_style = FillSolid ;
    XChangeGC(X->dpy, gc, gc_mask, &gc_val) ;

    XDrawLine(X->dpy, X->xid[(int) svtype], gc, x1, y1, x2, y2) ;
}


void
xnumimage(x, y, width, height, src)
int src, x, y, width, height ;
{
    XGCValues gc_val;
    unsigned long gc_mask;

    gc_mask = GCFunction | GCStipple | GCTileStipXOrigin | GCTileStipYOrigin ;
    gc_val.function = GXcopy ;
    gc_val.stipple = X->ixid[src] ;
    gc_val.ts_x_origin = x ;
    gc_val.ts_y_origin = y ;
    XChangeGC(X->dpy, X->ropgc, gc_mask, &gc_val) ;
    XFillRectangle(X->dpy, X->xid[(int) CLOCK_WIN], X->ropgc,
		   x, y, width, height) ;
}


void   
xtext(ftype, x, y, str)
     enum scale_type ftype ;
     int x, y ;
     char *str ;
{
    XGCValues gc_val;
    int i = INDEX_FROM_FONT_TYPE(ftype);

    XFontSet fontset = (XFontSet) xv_get(X->font[i], FONT_SET_ID);

    gc_val.function = GXcopy;
    XChangeGC(X->dpy, X->gc, GCFunction, &gc_val);

    XmbDrawImageString(X->dpy, X->xid[CLOCK_WIN], fontset, X->gc,
		       x, y, str, strlen(str)) ;
}
