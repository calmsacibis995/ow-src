#ifndef lint
static char sccsid[]="@(#)clock.c	1.13 08/23/94 Copyright 1988-1990 Sun Microsystem, Inc." ;
#endif

/*
 *  The DeskSet clock     (independent routines).
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
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/time.h>
#include <pwd.h>
#include <math.h>
#include "patchlevel.h"
#include "clock.h"

/*  This is the data to construct the number for the digital clock.
 *  There are two components:
 *
 *  numpos  - an index into the numvals array for this number.
 *  numvals - the data array for the numbers. Each entry consists of an
 *            initial count which says how many components follow.
 */

const int numpos[] = {
 0, 7, 10, 16, 22, 27, 33, 40, 44, 52, 59
} ;

const int numvals[] = {
  6,  0,  1,  2,  3,  4,  5,        /* 0 */
  2,  6,  7,                        /* 1 */
  5,  8,  2,  9,  3, 14,            /* 2 */
  5,  8,  2,  4, 10, 14,            /* 3 */
  4,  1,  2,  4, 14,                /* 4 */
  5, 11,  1,  4,  5, 14,            /* 5 */
  6, 11,  1,  3,  4,  5, 14,        /* 6 */
  3,  8,  2,  4,                    /* 7 */
  7,  0,  1,  2,  3,  4,  5, 14,    /* 8 */
  6,  0,  1,  2,  4,  5, 14,        /* 9 */
  2, 12, 13,                        /* 10 */
} ;


/*  Set of values to compare the new clock height (after a resize), to
 *  determine the size of the fonts used to display the text values.
 */

const int scaleH[MAXSCALES] = { 50, 150, 300, 450 } ;

static int cs[360], sn[360] ;

Clock clk ;
Options options ;
ClockDisplay display ;

extern char *astrs[] ;      /* Alarm settings. */
extern char *clk_res[] ;    /* Clock resources. */
extern char *days[];        /* Days of the week. */
extern char *hrstrs[] ;     /* Hours. */
extern char *mess[] ;       /* Messages. */
extern char *months[];      /* Months. */
extern char *month_days[];  /* Days of the month. */
extern char *opts[] ;       /* Command line options. */
extern char *sstrs[] ;      /* Scales. */
extern char *ustrs[] ;      /* Usage message. */
extern char *vstrs[] ;      /* Various. */
extern const int Sfscale[] ;              /* Sizes for seconds font at each scale. */
extern const int DTfscale[] ;             /* Sizes for date/timezone font. */


void
adjust_date_size()    /* Shrink or expand window depending upon date. */
{
    int i = INDEX_FROM_FONT_TYPE( DTFONT );

    if (DATE_ON(options))
	clk->cheight -= (display->fheight[i] + GAP) ;
    else
	clk->cheight += (display->fheight[i] + GAP) ;
    set_frame_hints() ;
}


void
adjust_timezone_size()   /* Shrink or expand window depending upon timezone. */
{
    int i = INDEX_FROM_FONT_TYPE( DTFONT );

    if (options->tzval == TRUE)
	clk->cheight -= (display->fheight[i] + GAP) ;
    else
	clk->cheight += (display->fheight[i] + GAP) ;
    set_frame_hints() ;
}


void
analog_repaint(ftype)
     Face ftype ;
{
  int prw, prh, smaller ;

  if (ftype == ICON_ANALOG)
    {
      init_icon_images() ;
      ximage(SURFACE, CLOCK_ICON, 0, 0, 64, 64,
             GSRC, (options->icontype == ROMAN) ? ICONROMAN : ICONANALOG) ; 
      paint_hands(SURFACE, CLOCK_ICON, clk->mins * 6,
                  clk->hours * 30 + clk->mins / 2, 64) ;

    }
  else
    { 
      smaller = (clk->width < clk->cheight) ? clk->width : clk->cheight ;
      init_analog(smaller, smaller) ;
      if ( options->repaint_type == RESIZE_REPAINT )
	  xclear(SURFACE, CLOCK_WIN, 0, 0, clk->width, clk->height) ;
      prw = display->hands_width ;
      prh = display->hands_height ;
      xclear(CLOCK_IMAGE, HANDSPR, 0, 0, prw, prh) ;
      paint_hands(CLOCK_IMAGE, HANDSPR, clk->mins * 6,
		  clk->hours * 30 + clk->mins / 2, prw) ;
      ximage(CLOCK_IMAGE, HANDSPR, 0, 0, prw, prh, GOR, DOTSPR) ;
      center(clk->width, clk->cheight,
             &display->centerX, &display->centerY, prw, prh) ;
      ximage(SURFACE, CLOCK_WIN,
             display->centerX, display->centerY, prw, prh, GSRC, HANDSPR) ;
    }
  if (SECONDS_ON(options))
      paint_second_hand() ;
}


int
armwidth(r)
     int r ;
{
  int w ;
  float fudge = 1.0 + (20.0 / r) ;
 
  if (fudge > 1.6) fudge = 1.6 ;
  w = (int) rint((double) fudge * r / 8.0) ;
  if (w % 2 == 0)
      w++ ;
  return(w) ;
}


void
center(cwidth, cheight, x, y, w, h)
int cwidth, cheight ;
int *x, *y ;
int w, h ;
{
  *x = (cwidth  - w) / 2 ;
  *y = (cheight - h) / 2 ;
}


int
check_alarm_setting(setting)
char *setting ;
{
  int len = 0 ;
  int n ;

  if (setting != NULL) len = strlen(setting) ;
  for (n = 0; n < len; n++)
    if (isupper(setting[n])) setting[n] = tolower(setting[n]) ;

       if (EQUAL(setting, astrs[(int) A_NONE]))  options->achoice = A_NONE ;
  else if (EQUAL(setting, astrs[(int) A_ONCE]))  options->achoice = A_ONCE ;
  else if (EQUAL(setting, astrs[(int) A_DAILY])) options->achoice = A_DAILY ; 
  else 
    {  
      options->achoice = A_NONE ; 
      return(FALSE) ;
    }
  return(TRUE) ;
}


void
dig_repaint()
{
  char dtime[5] ;                          /* Digital values for time. */
  int cwidth, height, hval, i, width, x, y ;

  init_digital(clk->width, clk->cheight) ;
  if ( options->repaint_type == RESIZE_REPAINT )
      xclear(SURFACE, CLOCK_WIN, 0, 0, clk->width, clk->height) ;
  display->x = x = (clk->width -
             (6 * display->scale * (MIN_FONT_WIDTH + MIN_FONT_SPACE))) / 2 ;
  if (x < GAP) display->x = x = GAP ;
  display->y = y = (clk->cheight - (MIN_FONT_HEIGHT * display->scale)) / 2 ;
  width  = MIN_FONT_WIDTH  * display->scale ;
  height = MIN_FONT_HEIGHT * display->scale ;

  hval = clk->hours ;
  if (options->schoice == CS_NONE)
    {
      if (hval == 0 && options->digtype == HOURS12) hval = 12 ;
      if (hval > 12 && options->digtype == HOURS12) hval -= 12 ;
    }
  SPRINTF(dtime, "%02d%c%02d", hval, COLON, clk->mins) ;
  if (dtime[0] == '0') dtime[0] = SPACE ;

  cwidth = MIN_FONT_WIDTH * display->scale ;
  for (i = 0; i < 5; i++)
    xnumimage(x + (i * cwidth) + (i * MIN_FONT_SPACE * display->scale),
              y, width, height, dtime[i] - '0') ;
  paint_dig_seconds() ;
}


void
do_char(val)      /* Handle keyboard accelerator option. */
char val ;
{
  init_options() ;
  switch (val)
    {
      case CHAR_12HOUR : options->digtype = HOURS12 ;
			if ( DIGITAL_ON(options) )
			      get_font(Sfscale[(int) display->OLscale],  display->OLscale, SFONT) ;
	  break ;
      case CHAR_24HOUR : options->digtype = HOURS24 ;
                         break ;
      case CHAR_CLOCK  : if (options->schoice != CS_NONE)
                           {
                             options->schoice = CS_NONE ;
                             set_stopwatch_val(options->schoice) ;
                             handle_time(options->face) ;
                           }
                         else if (options->face == ANALOG)
                           options->face = DIGITAL ;
                         else options->face = ANALOG ;
                         set_frame_hints() ;
                         set_min_frame_size() ;
                         break ;
      case CHAR_DATE   : options->date = !options->date ;
                         set_frame_hints() ;
                         set_min_frame_size() ;
                         adjust_date_size() ;
	  		 if ( DATE_ON(options) )
			     get_font(DTfscale[display->OLscale],  display->OLscale, DTFONT) ;
                         break ;
      case CHAR_ICON   : if (options->icontype == NORMAL)
                           options->icontype = ROMAN ;
                         else options->icontype = NORMAL ;
                         break ;
      case CHAR_SECS   : if (options->seconds == TRUE)
                           options->seconds = FALSE ;
                         else { 
			     options->seconds = TRUE ;
			     if ( DIGITAL_ON(options) )
				 get_font(Sfscale[(int) display->OLscale],  display->OLscale, SFONT) ;
			 }
                         break ;
      case CHAR_STOP   : options->testing = 0 ;
                         if (options->schoice == CS_NONE ||
                             options->schoice == CS_STOP)
                           options->schoice = CS_RESET ;
                         else if (options->schoice == CS_RESET)
                           options->schoice = CS_START ;
                         else if (options->schoice == CS_START)
                           options->schoice = CS_STOP ;
                         set_stopwatch_val(options->schoice) ;
                         do_stopwatch(options->schoice) ;
                         return ;
      case CHAR_TEST   : options->schoice = CS_NONE ;
                         options->testing = !options->testing ;
                         clk->tm = NULL ;
                         handle_time(options->face) ;
                         handle_timer() ;
                         break ;
      case CHAR_TZONE  : options->tzval = !options->tzval ;
                         set_frame_hints() ;
                         set_min_frame_size() ;
                         set_tzval(options->tzval) ;
                         break ;
      case CHAR_QUIT   : exit(0) ;
    }
  set_option_values() ;   /* Set property items from option values. */
  options->savedFace = options->face ;
}


void
do_clock(argc, argv)
int argc ;
char *argv[] ;
{
  char *ptr ;
  int i ;

  clk->latest_digital_w = -1 ;
  clk->latest_digital_h = -1 ;
  display->progname = argv[0] ;              /* Save programs name. */
  display->OLscale = S_MEDIUM ;
  display->iheight = display->iwidth = -1 ;  /* Signifies to initial size. */

  init_cmdline_opts() ;    /* Initialise command line text strings. */
 
  if ((ptr = strrchr(argv[0], '/')) != NULL)
    read_str(&options->appname, ptr+1) ;
  else read_str(&options->appname, argv[0]) ;
 
/*  Search through all the command line arguments, looking for -name.
 *  If it's present, then this name with be used, when looking for X resources
 *  for this application. When the rest of the command line arguments are
 *  checked later on, then the -name argument (if found) is ignored.
 */
   
  for (i = 0; i < argc; i++)
    if (EQUAL(argv[i], opts[O_NAME]))
      {
        if ((i+1) > argc) usage(argv[0]) ;
        read_str(&options->appname, argv[i+1]) ;
        break ;
      }

  make_graphics(&argc, argv) ; /* Allocate memory for graphics objects. */

  init_text() ;            /* Setup text strings depending upon language. */
  init_points() ;
  init_opt_vals() ;            /* Initialise options to default values. */

/*  See if the TZ environment variable is set, and if so, use this value as
 *  the remote timezone to monitor. Note that this can be overridden by
 *  either the appropriate X resource, or the -TZ command line option.
 */

  if ((ptr = getenv("TZ")) != NULL)
    {
      read_str(&options->localtzname, ptr) ;
      read_str(&options->tzname, ptr) ;
      options->tzval = TRUE ;
    }

  load_resources() ;           /* Get resources from various places. */
  read_resources(*argv) ;      /* Read resources from merged database. */
  get_options(argc, argv) ;
  options->savedFace = options->face ;

  if (display->iwidth != -1 && display->iheight != -1)
    for (i = 0; i < MAXSCALES; i++)
      if (display->iheight > scaleH[i])
        display->OLscale = (enum scale_type) i ;

  make_frame() ;
  init_gray_patch() ;
  if ( USE_DTFONT(options) )
      get_font(DTfscale[(int) display->OLscale], display->OLscale, DTFONT) ;
  if ( USE_SFONT(options) )
      get_font(Sfscale[(int) display->OLscale], display->OLscale, SFONT) ;
  make_gcs() ;

  init_numbers() ;
  init_icon() ;
  init_display() ;
  make_canvas() ;

  set_timezone(options->tzname) ;
  get_time() ;
  clk->hours = clk->tm->tm_hour ;
  clk->mins  = clk->tm->tm_min ;
  clk->secs  = clk->tm->tm_sec ;

  set_def_dims() ;
  set_frame_hints() ;
  handle_timer() ;
  write_cmdline() ;       /* Save current state in command line. */

  display->secondhand.lastSecX = -1 ;
  start_tool() ;
}


void
do_stopwatch(setting)     /* Update stopwatch display. */
StopType setting ;
{
  if (setting == CS_NONE)
    xclear(SURFACE, CLOCK_WIN, 0, 0, clk->width, clk->height) ;
  if (setting == CS_RESET) clk->hours = clk->mins = clk->secs = 0 ;

  /* make sure seconds font is loaded */
  get_font(Sfscale[(int) display->OLscale], display->OLscale, SFONT) ;

  handle_timer() ;
  if (setting == CS_NONE) handle_time(options->face) ;
  handle_repaint(options->face) ;
}


void
erase_second_hand()
{
  enum sv_type svtype ;
  int x1, y1, x2, y2 ;

  if (options->face == ICON_ANALOG) svtype = CLOCK_ICON ;
  else                              svtype = CLOCK_WIN ;

/* Burn the last displayed second hand off */

  x1 = display->secondhand.lastSecX ;
  y1 = display->secondhand.lastSecY ;
  x2 = display->secondhand.lastSecX1 ;
  y2 = display->secondhand.lastSecY1 ;
  if (x1 != -1) xline(SURFACE, svtype, GXOR, x1, y1, x2, y2) ;
}


int
get_bool_resource(rtype, boolval)   /* Get boolean resource from the server. */
enum res_type rtype ;
int *boolval ;
{
  char *val, tempstr[MAXSTRING] ;
  int len, n ;

  if ((val = get_resource(rtype)) == NULL) return(0) ;
  STRCPY(tempstr, val) ;
  len = strlen(tempstr) ;
  for (n = 0; n < len; n++)
    if (isupper(tempstr[n])) tempstr[n] = tolower(tempstr[n]) ;
  if (EQUAL(tempstr, vstrs[(int) V_TRUE])) *boolval = TRUE ;
  else                                     *boolval = FALSE ;
  return(1) ;
}


int
get_cmdstr(str, argv)
char **str, **argv ;
{
  if (*str != NULL) FREE(*str) ;
  *str = NULL ;
  argv++ ;
  if (*argv == NULL || CMD_ARG("-")) return(FALSE) ;
  else
    {
      *str = (char *) malloc((unsigned) (strlen(*argv) + 1)) ;
      STRCPY(*str, *argv) ;
    }
  return(TRUE) ;
}


void
get_font_scale(s)
char *s ;
{
       if (EQUAL(s, sstrs[(int) S_SMALL]))  display->OLscale = S_SMALL ;
  else if (EQUAL(s, sstrs[(int) S_MEDIUM])) display->OLscale = S_MEDIUM ;
  else if (EQUAL(s, sstrs[(int) S_LARGE]))  display->OLscale = S_LARGE ;
  else if (EQUAL(s, sstrs[(int) S_EXTRALARGE]))
    display->OLscale = S_EXTRALARGE ;
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


void
get_options(argc, argv)   /* Extract command line options. */
int argc ;
char **argv ;
{
  char *progname ;
  char *tempstr = NULL ;

  progname = *argv ;       /* Save program name for usage(). */
  argc-- ;                 /* Skip program name. */
  argv++ ;
 
  while (*argv && argc > 0)
    {
           if (CMD_ARG(opts[(int) O_MWN]))   options->title    = FALSE ;
      else if (CMD_ARG(opts[(int) O_PWN]))   options->title    = TRUE ;
      else if (CMD_ARG(opts[(int) O_T]))     options->testing  = TRUE ;
      else if (CMD_ARG(opts[(int) O_R]))     options->icontype = ROMAN ;
      else if (CMD_ARG(opts[(int) O_12]))    options->digtype  = HOURS12 ;
      else if (CMD_ARG(opts[(int) O_24]))    options->digtype  = HOURS24 ;
      else if (CMD_ARG(opts[(int) O_ANAL]))  options->face     = ANALOG ;
      else if (CMD_ARG(opts[(int) O_DIG]))   options->face     = DIGITAL ;
      else if (CMD_ARG(opts[(int) O_PDATE])) options->date     = TRUE ;
      else if (CMD_ARG(opts[(int) O_MDATE])) options->date     = FALSE ;
      else if (CMD_ARG(opts[(int) O_PSEC]))  options->seconds  = TRUE ;
      else if (CMD_ARG(opts[(int) O_MSEC]))  options->seconds  = FALSE ;
      else if (CMD_ARG(opts[(int) O_V])     ||
               CMD_ARG(opts[(int) O_QUE])   ||
               CMD_ARG(opts[(int) O_HELP])) usage(progname) ;
      else if (CMD_ARG(opts[(int) O_NAME]))
        {
          if (argc < 2) usage(progname) ;     /* Processed earlier. */
          INC ;
        }
      else if (CMD_ARG(opts[(int) O_TZ]))
        {
          if (get_cmdstr(&options->tzname, argv) == FALSE) usage(progname) ;
          INC ;
          options->userTZ = TRUE ;
          options->tzval = TRUE ;
        }
      else if (CMD_ARG(opts[(int) O_A]))
        {
          if (get_cmdstr(&tempstr, argv) == FALSE) usage(progname) ;
          INC ;
          if (check_alarm_setting(tempstr) == FALSE)
            FPRINTF(stderr, mess[(int) M_AVAL], progname, tempstr) ;
        }
      else if (CMD_ARG(opts[(int) O_AT]))
        {
          if (get_cmdstr(&tempstr, argv) == FALSE) usage(progname) ;
          INC ;
          SSCANF(tempstr, "%d:%d", &options->ahrval, &options->aminval) ;

          if (options->ahrval < 0 || options->ahrval > MAXHRVAL)
            {
              FPRINTF(stderr, mess[(int) M_ATVAL], progname, tempstr) ;
              options->ahrval = -1 ;
            }
          if (options->aminval < 0 || options->aminval > MAXMINVAL)
            {
              FPRINTF(stderr, mess[(int) M_ATVAL], progname, tempstr) ;
              options->aminval = -1 ;
            }
        }
      else if (CMD_ARG(opts[(int) O_ACMD]))
        {
          if (get_cmdstr(&options->acmd, argv) == FALSE) usage(progname) ;
          INC ;
        }
      else if (CMD_ARG(opts[(int) O_HCMD]))
        {
          if (get_cmdstr(&options->hcmd, argv) == FALSE) usage(progname) ;
          INC ;
        } 
      else usage(progname) ;
      argc-- ;
      argv++ ;
    }

  check_args() ;        /* Check for size, label, scale and position. */
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
get_time()       /* Get the current time, or increment if testing. */
{
  int clock ;

  if (options->testing)
    {
      if (clk->tm == NULL)
        {
          clock = time((time_t *) 0) ;    /* Get local time from kernel. */
          clk->tm = localtime((time_t *) &clock) ;
        }
      if (++(clk->tm->tm_min) > MAXMINVAL)      /* Increment local time. */
        {
          clk->tm->tm_min = 0 ;
          if (++(clk->tm->tm_hour) > MAXHRVAL) clk->tm->tm_hour = 0 ;
        }
    }    
  else
    { 
      clock = time((time_t *) 0) ;
      clk->tm = localtime((time_t *) &clock) ;
    }
}


void
handle_repaint(ftype)
     Face ftype ;
{
    switch (ftype) {
    case ANALOG      :
    case ICON_ANALOG : analog_repaint(ftype) ;
	break ;
    case DIGITAL     : dig_repaint() ;
    }

    if (DATE_ON(options) && ftype != ICON_ANALOG)
	paint_date( ftype ) ;

    if (options->tzval == TRUE && IS_TIMEZONE(options) 
	&& ftype != ICON_ANALOG 
	&& options->repaint_type != TIMER_REPAINT
	)
	paint_timezone(DATE_ON(options)) ;

    /* reset repaint type */
    options->repaint_type = FULL_REPAINT;
}


void
handle_resize()
{
/*  If the clock canvases cheight value is nagative, then return immediately.
 *  This indicates that the frame has been reduced to a very small size, and
 *  subsequently enlarged to the minimum size, but the canvas hasn't had a
 *  chance to get the corresponding RESIZE event yet, so is currently too
 *  small, and if processed will result in negative values being passed to
 *  XFillPolygon, thereby causing a server crash (see below).
 */

  if (clk->cheight < 0) return ;
  display->secondhand.lastSecX = -1 ;
  
  /*
   * TANK:  set CANVAS_FIXED_IMAGE to FALSE so that the CANVAS_REPAINT_PROC
   * gets called for *all* resizes, so this is no longer necessary.  This fixes
   * the problem of repainting twice when the frame gets resized bigger.
   *
   handle_repaint(options->face) ;
   */


  /*
   * TANK: still want to tailor repainting special for resize events
   * because scaling may change.
   */
  options->repaint_type = RESIZE_REPAINT;
}


void
handle_time(ftype)
     Face ftype ;
{
    /* in process of resizing, don't paint! */
    if ( options->repaint_type == RESIZE_REPAINT )
	return;

    options->repaint_type = TIMER_REPAINT;

    if (options->schoice == CS_NONE)  paint_clock(ftype) ;
    else if (options->schoice == CS_START) paint_stopwatch(ftype) ;
    else handle_repaint(ftype) ;

    options->repaint_type = FULL_REPAINT;
}


void
handle_timer()
{
  if (options->testing == TRUE)
    set_timer(0, 1, 0, 1) ;                    /* Poll timer. */
  else if (options->schoice == CS_RESET || options->schoice == CS_STOP)
    set_timer(0, 0, 0, 0) ;                    /* Disable timer. */
  else if (SECONDS_ON(options))
    set_timer(1, 0, 1, 0) ;                    /* Increment in seconds. */
  else
    {
      get_time() ;
      set_timer(60 - clk->tm->tm_sec, 0, 60, 0) ;   /* Increment in minutes. */
    }
}


void
init_digital(width, height)     /* Initialise DIGITAL display. */
int width, height ;
{
  int scale ;
 
/*  Determine the maximum scale factor we can use, for this size window.
 *
 *  XXX: There is currently a bug with the XFillPolygon call that can
 *       crash the xnews server, if you pass it invalid (negative) parameters.
 *       Because of this, more sanity checks then are really necessary are
 *       performed on the scale value.
 */
 
  if (width  == clk->latest_digital_w &&    /* Don't redo if already done. */
      height == clk->latest_digital_h)
    return ;

  clk->latest_digital_w = width ;           /* Remember what did. */
  clk->latest_digital_h = height ;

  for (scale = 1; ; scale++)
    if ((scale * (MIN_FONT_WIDTH + MIN_FONT_SPACE) * 6 > clk->width) ||
        (scale * MIN_FONT_HEIGHT > clk->cheight)) break ;
  if (scale > 1 && scale < (SCREENHEIGHT / MIN_FONT_HEIGHT)) scale-- ;
  else scale = 1 ;
 
  if (display->scale != scale)
    {
      display->scale = scale ;
      grow_font(display->scale) ;
      width  = MIN_FONT_WIDTH * display->scale ;
      height = MIN_FONT_HEIGHT * display->scale ;
      build_numbers(width, height) ;
    } 
}


void
init_display()
{
  display->secondhand.lastSecX  = -1 ;
  display->secondhand.lastSecY  = -1 ;
  display->secondhand.lastSecX1 = -1 ;
  display->secondhand.lastSecY1 = -1 ;
  display->hands.angle1         = -1 ;
  display->hands.angle2         = -1 ;
  display->hands.width          = -1 ;
}


void
init_numbers()
{
  int i ;

  for (i = 0; i < 360; i++)
    {
      cs[i] = (int) rint(100.0 * cos(i * PI / 180)) ;
      sn[i] = (int) rint(100.0 * sin(i * PI / 180)) ;
    }
}


void
init_opt_vals()     /* Initialise options to default values. */
{
  options->face      = ANALOG ;
  options->icontype  = NORMAL ;
  options->digtype   = HOURS12 ;
  options->ahrval    = -1 ;
  options->aminval   = -1 ;
  options->achoice   = A_NONE ;
  options->schoice   = CS_NONE ;
  options->title     = TRUE ;
  options->dosleep      = TRUE ;
}


void
paint_clock(ftype)
     Face ftype ;
{
    if (options->schoice == CS_NONE) 
	get_time() ;

    if (clk->tm->tm_min != clk->mins || clk->tm->tm_hour != clk->hours)
	{

	    /* If this is a new hour value, then run hourly command (if set). */

	    if (clk->tm->tm_hour != clk->hours && !clk->tm->tm_min)
		if (options->hcmd != NULL) 
		    run_command(options->hcmd) ;
 
	    /*  If this is the alarm time, then run the alarm command (if set),
	     *  depending upon the alarm setting and the presence of an alarm command.
	     */
 
	    if (options->ahrval != -1  && clk->tm->tm_hour == options->ahrval &&
		options->aminval != -1 && clk->tm->tm_min  == options->aminval)
		if (options->achoice != A_NONE)
		    if (options->acmd != NULL)
			{
			    run_command(options->acmd) ;
			    if (options->achoice == A_ONCE)
				{
				    options->achoice = A_NONE ; /* Turn off alarm. */
				    set_option_values() ;
				}
			}
		    else beep() ;
 
	    clk->mins  = clk->tm->tm_min ;
	    clk->hours = clk->tm->tm_hour ;
	    clk->secs  = clk->tm->tm_sec ;
	    handle_repaint(ftype) ;
	}
    else if (SECONDS_ON(options))
	{
	    clk->secs = clk->tm->tm_sec ;
	    if (ftype == DIGITAL)
		paint_dig_seconds() ;
	    else
		{ 
		    erase_second_hand() ;
		    paint_second_hand() ;
		}
	}
}


void
paint_date( ftype )
     Face ftype;
{
    time_t old_time;
    struct tm old_tm;
    int i = INDEX_FROM_FONT_TYPE( DTFONT );
 
    xclear(SURFACE, CLOCK_WIN, 0, clk->cheight,
	   clk->width, display->fheight[i]) ;
    old_tm = *(clk->tm);
    get_time() ;

  
    /* if timer repaint, only update date if it changed */
    if ( (ftype == DIGITAL) || (options->repaint_type != TIMER_REPAINT)
	|| (old_tm.tm_mday != clk->tm->tm_mday)
	|| (ftype == ANALOG)
	) {
	int strwidth ;
	char buf[DATEBUFLEN] ;

	strftime(buf, DATEBUFLEN, "%x", clk->tm);
	strwidth = get_str_width(DTFONT, buf) ;
	xtext(DTFONT, (clk->width - strwidth) / 2,
	      clk->cheight + display->fascent[i], buf) ;
    }
}


void
paint_dig_seconds()
{
    char secs[3] ;
    int x ;
    int i;

    if (display->y <= 0) 
	return ;

    x = display->x + (5 * (MIN_FONT_WIDTH + MIN_FONT_SPACE) * display->scale) ;
    SPRINTF(secs, "%02d", clk->secs) ;

    i = INDEX_FROM_FONT_TYPE( SFONT );
    if (SECONDS_ON(options))
	xtext(SFONT, x, display->y + display->fheight[i], secs) ;
    if ( USE_SFONT(options) )
        get_font(Sfscale[(int) display->OLscale],  display->OLscale, SFONT) ;
    if ( USE_DTFONT(options) )
        get_font(DTfscale[(int) display->OLscale], display->OLscale, DTFONT) ;

     if ( options->digtype == HOURS12 )
	xtext(SFONT, x, display->y + (2 * display->fheight[i]),
	      (clk->hours < 12) ? hrstrs[(int) H_AM] : hrstrs[(int) H_PM]) ;
}


void
paint_hands(dtype, svtype, angle1, angle2, w)
     enum drawing_type dtype ;
     enum sv_type svtype ;
     int angle1 ;             /* Long hand */
     int angle2 ;             /* Short hand */
     int w ;                  /* Canvas width */
{
    register int x1, y1, yy1, x2, y2, x3, y3 ;
    register int fromrim, topext, leftext ;
 
    /* Cache hands positions for erasing later. perf mod. */
 
    display->hands.angle1 = angle1 ;
    display->hands.angle2 = angle2 ;
    display->hands.width = w ;
 
    fromrim = (FROMRIM * w) / 128 ;
    leftext = (LEFTEXT * w) / 128 ;
    topext  = (TOPEXT  * w) / 128 ;
 
    x1 = w / 2 ;		/* Tip of hand */
    y1 = fromrim ;
    yy1 = x1 - (2 * (w / 2 - fromrim)) / 3 ;
 
    x2 = x1 - leftext ;		/* Lower left hand of hand */
    y2 = x1 + topext ;
   
    x3 = x1 + leftext ;		/* Lower right hand of hand */
    y3 = x1 + topext ;
   
    paint_hand(dtype, svtype, x1, yy1, x2, y2, x3, y3, angle2, w) ; /* hour */
    paint_hand(dtype, svtype, x1, y1,  x2, y2, x3, y3, angle1, w) ; /* minute */
}


void
paint_second_hand()
{
  enum sv_type svtype ;
  int x, y, diameter, radius, fromrim, angle, height, width ;

  if (!SECONDS_ON(options))
      return ;

  angle = clk->secs * 6 ;

  if (options->face == ICON_ANALOG)
    {
      svtype = CLOCK_ICON ;
      width = height = diameter = 64 ;
    }
  else
    { 
      svtype   = CLOCK_WIN ;
      width    = clk->width ;
      height   = clk->cheight ;
      diameter = display->hands_width ;
    }
  radius  = diameter / 2 ;
  fromrim = (FROMRIM * diameter) / 128 ;
  x = rotx(radius, fromrim, radius, angle) ;
  y = roty(radius, fromrim, radius, angle) ;

/* Cache new second, then paint. */

  display->secondhand.lastSecX = width / 2 ;
  display->secondhand.lastSecY = height / 2 ;
  display->secondhand.lastSecX1 = display->centerX + x ;
  display->secondhand.lastSecY1 = display->centerY + y ;

  if (options->face == ICON_ANALOG && !options->use_wincol)
    {
      xline(SURFACE, svtype, GXOR, width / 2, height / 2,
            display->centerX + x, display->centerY + y) ;
    }
  else
    xline(SURFACE, svtype, GXOR,
          width / 2, height / 2, display->centerX + x, display->centerY + y) ;
}


void
paint_stopwatch(ftype)
     Face ftype ;
{
  int dorepaint = 0 ;

  if (options->schoice == CS_START && !options->doingprops &&
                                      !options->doingrepaint)
    {
      ++clk->secs ;
      if (clk->secs > MAXSECVAL)
        {
          ++clk->mins ;
          clk->secs = 0 ;
          dorepaint = 1 ;
        }
      if (clk->mins > MAXMINVAL)
        {
          ++clk->hours ;
          clk->mins = 0 ;
          dorepaint = 1 ;
        } 
      if (clk->hours > MAXHRVAL) clk->hours = 0 ;
    } 
  if (dorepaint) handle_repaint(ftype) ;
  else
    {
      if (ftype == DIGITAL) paint_dig_seconds() ;
      else
        { 
          erase_second_hand() ;
          paint_second_hand() ;
        }
    }

/* Check for possible hourly command or alarm command expiration. */

  if (options->hcmd == NULL && options->acmd == NULL) return ;
  get_time() ;

/* If this is a new hour value, then run hourly command (if set). */

  if (!clk->tm->tm_min && !clk->tm->tm_sec)
    if (options->hcmd != NULL) run_command(options->hcmd) ;

/*  If this is the alarm time, then run the alarm command (if set),
 *  depending upon the alarm setting and the presence of an alarm command.
 */

  if (options->ahrval != -1  && clk->tm->tm_hour == options->ahrval &&
      options->aminval != -1 && clk->tm->tm_min  == options->aminval &&
                                clk->tm->tm_sec  == 0)
    if (options->achoice != A_NONE)
      if (options->acmd != NULL)
        {
          run_command(options->acmd) ;
          if (options->achoice == A_ONCE)
            {
              options->achoice = A_NONE ;    /* Turn off alarm. */
              set_option_values() ;
            }
        }
      else beep() ;
}


void
paint_ticks(dest, radius, src)
     enum sv_type dest, src ;
     int radius ;
{
  int i ;
  int x, y ;
  int arm_width = armwidth(radius) ;
 
  for (i = 0; i < 12; i++)
    {
      x = cs[i*30] * 20 * radius / 2400 + radius - arm_width / 4 ;
      y = sn[i*30] * 20 * radius / 2400 + radius - arm_width / 4 ;
      ximage(CLOCK_IMAGE, dest, x, y, (arm_width/2) + 1, (arm_width/2) + 1,
             GOR, src) ;
    }
}


void
paint_timezone(date_displayed)
int date_displayed ;
{
  int strwidth ;             /* Width in pixels of timezone name. */
  int y ;                    /* Y position for time zone text. */
  int i = INDEX_FROM_FONT_TYPE(DTFONT);

  y = clk->cheight + GAP ;

  if (date_displayed) 
      y += display->fheight[i] + GAP ;

  xclear(SURFACE, CLOCK_WIN, 0, y, clk->width, display->fheight[i]) ;

  strwidth = get_str_width(DTFONT, options->tzname) ;
  y = clk->cheight + display->fascent[i] + GAP ;

  if (date_displayed)
      y += display->fheight[i] + GAP ;

  xtext(DTFONT, (clk->width - strwidth) / 2, y, options->tzname) ;
}


void
read_resources(progname)      /* Read all possible resources from database. */
char *progname ;
{
  int   boolval, intval ;
  char  strval[MAXSTRING] ;

  if (get_bool_resource(R_CFACE, &boolval))
    options->face     = (boolval) ? ANALOG  : DIGITAL ;

  if (get_bool_resource(R_IFACE, &boolval))
    options->icontype = (boolval) ? NORMAL  : ROMAN ;

  if (get_bool_resource(R_DIGMODE, &boolval))
    options->digtype  = (boolval) ? HOURS12 : HOURS24 ;

  if (get_bool_resource(R_LOCAL,   &boolval)) options->tzval      = !boolval ;
  if (get_bool_resource(R_ICOLOR,  &boolval)) options->use_wincol =  boolval ;
  if (get_bool_resource(R_SECHAND, &boolval)) options->seconds    =  boolval ;
  if (get_bool_resource(R_DATE,    &boolval)) options->date       =  boolval ;

  if (get_str_resource(R_TZONE, strval)) read_str(&options->tzname, strval) ;

  if (get_int_resource(R_AHRVAL, &intval))
    {
      options->ahrval = intval ;
      if (options->ahrval < -1 || options->ahrval > MAXHRVAL)
        {
          FPRINTF(stderr, mess[(int) M_RESD], 
                          progname, clk_res[(int) R_AHRVAL], intval) ;
          options->ahrval = -1 ;
        }
    }    

  if (get_int_resource(R_AMINVAL, &intval))
    {
      options->aminval = intval ;
      if (options->aminval < -1 || options->aminval > MAXMINVAL)
        {
          FPRINTF(stderr, mess[(int) M_RESD],
                          progname, clk_res[(int) R_AMINVAL], intval) ;
          options->aminval = -1 ;
        }
    }    

  if (get_str_resource(R_ACHOICE, strval))
    {
      if (check_alarm_setting(strval) == FALSE)
        FPRINTF(stderr, mess[(int) M_RESS],
                        progname, clk_res[(int) R_ACHOICE], strval) ;
    }

  if (get_str_resource(R_ACMD, strval)) read_str(&options->acmd, strval) ;
  if (get_str_resource(R_HCMD, strval)) read_str(&options->hcmd, strval) ;

  if (get_str_resource(R_DTFONT, strval))
    read_str(&display->fontnames[(int) DTFONT], strval) ;
  if (get_str_resource(R_SFONT,  strval))
    read_str(&display->fontnames[(int) SFONT],  strval) ;

  if (get_bool_resource(R_TITLE, &boolval)) options->title   = boolval ;
  if (get_bool_resource(R_SLEEP, &boolval)) options->dosleep = boolval ;
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


int
rotx(x, y, r, th)                       /* th is in degrees */
int x, y ;
int r, th ;
{
  float th1 ;
 
  th1 = (th * 2.0 * PI) / 360.0 ;
  return((int) ((x - r) * cos(th1) - (y - r) * sin(th1) + r)) ;
}
 
 
int
roty(x, y, r, th)                       /* th is in degrees */
int x, y ;
int r, th ;
{
  float th1 ;
 
  th1 = (th * 2.0 * PI) / 360.0 ;
  return((int) ((x - r) * sin(th1) + (y - r) * cos(th1) + r)) ;
}


int
run_command(cmd)         /* Fork off and exec a command. */
char *cmd ;
{
  int argc ;
  int len ;
  char *argv[50] ;
  char expanded[MAXPATHLEN] ;
  char *r_p, *s_p, *t_p ;
  char tsep ;

/*  The command has to be separated into tokens and setup in an *argv[]
 *  structure for the execvp call. Tokens are space separated, but
 *  allowance has to be made for quoted strings.
 */

  argc = 0 ;
  ds_expand_pathname(cmd, expanded) ;    /* Expand environment variables. */
  s_p = expanded ;
  while (s_p != NULL)
    {
           if (*s_p == '\'') tsep = '\'' ;
      else if (*s_p == '"')  tsep = '"' ;
      else                   tsep = ' ' ;
      r_p = s_p ;
      if (tsep != ' ')
        {
          s_p++ ;
          r_p++ ;
        }
      t_p = r_p ;
      while (t_p && *t_p != tsep && *t_p != '\0') t_p++ ;
      if (t_p != r_p)
        {
          len = t_p - s_p + 1 ;
          if (tsep != ' ') len++ ;
          argv[argc] = (char *) malloc((unsigned) len) ;
          STRNCPY(argv[argc], s_p, len) ;
          argv[argc][len-1] = '\0' ;
          argc++ ;
          s_p = t_p + 1 ;
        }
      else s_p = NULL ;
      if (t_p && *t_p == '\0') break ;
    }
  argv[argc] = 0 ;
  clock_exec(argv) ;
}


char *
set_bool(value)
int value ;
{
  return((value) ? vstrs[(int) V_TRUE] : vstrs[(int) V_FALSE]) ;
}


void
set_clock_props()
{
  if (options->schoice == CS_NONE)
    {
      set_timezone(options->tzname) ;
      write_cmdline() ;
    }

  /*
   * forego this, as the RESIZE_REPAINT should clear.
  xclear(SURFACE, CLOCK_WIN, 0, 0, clk->width, clk->height) ;
  */

  handle_timer() ;
  handle_time(options->face) ;


  /*
   * Handle repaint as if there was a resize (i.e. clear the
   * clock face).  this is because the properties can do radical
   * things to the display.  Note that handle_time() will set the
   * repaint_type to FULL_REPAINT, so we must do this afterward
   * to override this.
   */
  options->repaint_type = RESIZE_REPAINT;

  handle_repaint(options->face) ;
}


void
set_min_frame_size()
{
  int fx, fy, fw, fh ;
  int min_height, min_width ;
  int resize = FALSE ;
  int twd ;                  /* Width in pixels of the timezone name. */
  int i = INDEX_FROM_FONT_TYPE( DTFONT );

  get_frame_size(&fx, &fy, &fw, &fh) ;

/*  Determine if the user has shrunk the window below the minimum size.
 *  The minimum size is dependent upon analog or digital mode, and whether
 *  the date and/or a remote timezone are currently being displayed.
 */
 
  min_height = ((options->face == DIGITAL) ? MIN_DIG_HEIGHT
                                           : MIN_ANALOG_HEIGHT)
               + display->fheight[i] + GAP +
               (DATE_ON(options) ? display->fheight[i] + GAP
                                 : 0) ;
  min_width  = ((options->face == DIGITAL)
               ? 6 * (MIN_FONT_WIDTH + MIN_FONT_SPACE) : 64) ;

  if (options->tzval == TRUE && IS_TIMEZONE(options))
    {
      twd = get_str_width(DTFONT, options->tzname) ;
      twd += (2 * GAP) ;
      if (min_width < twd) min_width = twd ;
    }

  if (fw < min_width)
    {
      fw = min_width ;
      resize = TRUE ;
    } 
  if (fh < min_height)
    {
      fh = min_height ;
      resize = TRUE ;
    } 
  if (resize == TRUE) set_frame_size(fx, fy, fw, fh) ;
}


void
set_timezone(timezone)
char *timezone ;
{
  static char tzenv[MAXPATHLEN] ;

  if (options->tzval == TRUE && IS_TIMEZONE(options))
    { 
      SPRINTF(tzenv, vstrs[(int) V_TZ], timezone) ;
      PUTENV(tzenv) ;
      tzset() ;
    }
  else
    if (options->localtzname != NULL)
      {
        SPRINTF(tzenv, vstrs[(int) V_TZ], options->localtzname) ;
        PUTENV(tzenv) ;
        tzset() ;
      }
    else
#ifdef SVR4
      tzset() ;
#else
      tzsetwall() ;
#endif /*SVR4*/
}


void
usage(prog)
char *prog ;
{
  int i ;

  FPRINTF(stderr, ustrs[0], prog, PATCHLEVEL) ;
  FPRINTF(stderr, ustrs[1], prog) ;
  for (i = 2; i < MAXUSAGE; i++) FPUTS(ustrs[i], stderr) ;
  exit(1) ;
}


void
write_cmdline()        /* Save current status in clock command line. */
{
  int argc, i ;
  char *argv[MAXCMDOPTS], buf[255] ;

  argc = 0 ;
  for (i = 0; i < MAXCMDOPTS; i++) argv[i] = NULL ;

  if (options->title == TRUE) ADD_CMD(O_PWN) ;
  else                        ADD_CMD(O_MWN) ;

  if (options->testing  == TRUE) ADD_CMD(O_T) ;

  if (options->icontype == ROMAN) ADD_CMD(O_R) ;

  if (options->digtype == HOURS12) ADD_CMD(O_12) ;
  else                             ADD_CMD(O_24) ;

  if (options->face == DIGITAL) ADD_CMD(O_DIG) ;
  else                          ADD_CMD(O_ANAL) ;

  if (options->date == TRUE) ADD_CMD(O_PDATE) ;
  else                       ADD_CMD(O_MDATE) ;

  if (options->seconds == TRUE) ADD_CMD(O_PSEC) ;
  else                          ADD_CMD(O_MSEC) ;

  if (options->tzval  == TRUE &&
      options->userTZ == TRUE && IS_TIMEZONE(options))
    {
      ADD_CMD(O_TZ) ;
      ADD_CMDS(options->tzname) ;
    }

  if (options->acmd != NULL)
    {
      ADD_CMD(O_ACMD) ;
      ADD_CMDS(options->acmd) ;
    }

  if (options->hcmd != NULL)
    {
      ADD_CMD(O_HCMD) ;
      ADD_CMDS(options->hcmd) ;
    }

  if (options->achoice != A_NONE)
    {
      ADD_CMD(O_A) ;
      if (options->achoice == A_ONCE) ADD_CMD(O_AO) ;
      else                            ADD_CMD(O_AD) ;
    }

  if (options->ahrval != -1 && options->aminval != -1)
    {
      ADD_CMD(O_AT) ;
      SPRINTF(buf, "%1d:%1d", options->ahrval, options->aminval) ;
      read_str(&argv[argc++], buf) ;
    }

  save_cmdline(argc, argv) ;

  for (i = 0; i < argc; i++)                    /* Free up memory. */
    if (argv[i] != NULL) FREE(argv[i]) ;
}


void
write_resources()
{
  char intval[10] ;     /* For converting integer value. */
  char *value ;

  load_deskset_defs() ;
  put_resource(R_CFACE,   set_bool(options->face       == ANALOG)) ;
  put_resource(R_IFACE,   set_bool(options->icontype   == NORMAL)) ;
  put_resource(R_DIGMODE, set_bool(options->digtype    == HOURS12)) ;
  put_resource(R_LOCAL,   set_bool(options->tzval      == FALSE)) ;
  put_resource(R_SECHAND, set_bool(options->seconds    == TRUE)) ;
  put_resource(R_DATE,    set_bool(options->date       == TRUE)) ;
  put_resource(R_ICOLOR,  set_bool(options->use_wincol == TRUE)) ;
  put_resource(R_TITLE,   set_bool(options->title      == TRUE)) ;
  put_resource(R_SLEEP,   set_bool(options->dosleep    == TRUE)) ;

  if (display->fontnames[(int) DTFONT] != NULL)
    put_resource(R_DTFONT, display->fontnames[(int) DTFONT]) ;
  if (display->fontnames[(int) SFONT]  != NULL)
    put_resource(R_SFONT,  display->fontnames[(int) SFONT]) ;

  put_resource(R_TZONE, (IS_TIMEZONE(options))  ? options->tzname : "") ;
  put_resource(R_ACMD,  (options->acmd != NULL) ? options->acmd   : "") ;
  put_resource(R_HCMD,  (options->hcmd != NULL) ? options->hcmd   : "") ;

  SPRINTF(intval, "%d", options->ahrval) ;
  put_resource(R_AHRVAL, intval) ;

  SPRINTF(intval, "%d", options->aminval) ;
  put_resource(R_AMINVAL, intval) ;

       if (options->achoice == A_NONE)  value = astrs[(int) A_NONE] ;
  else if (options->achoice == A_ONCE)  value = astrs[(int) A_ONCE] ;
  else if (options->achoice == A_DAILY) value = astrs[(int) A_DAILY] ;
  put_resource(R_ACHOICE, value) ;

  save_resources() ;
}
