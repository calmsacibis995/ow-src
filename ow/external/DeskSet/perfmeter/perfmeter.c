#ifndef lint
static char sccsid[] = "@(#)perfmeter.c 1.9 92/10/27 Copyright (c) Sun Microsystems Inc." ;
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

#include "perfmeter.h"
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/vmmeter.h>
#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>
#include <netdb.h>
#include <sys/wait.h>
#include "clockhands.h"
#include "patchlevel.h"

#ifndef SVR4
#include <sys/dk.h>
#endif /*!SVR4*/

#ifdef SVR4
#include <rpc/clnt_soc.h>    /* UDPMSGSIZE has moved to a different include */
#endif /*SVR4*/

struct timeval TIMEOUT = { 20, 0 } ;

struct meter meters_init[] = {

/*
 *  name rname maxmax minmax curmax scale lastval showing resizing ceiling val
 */

{  NULL,  NULL,    100, 100,   100,    1,   -1,      0,      0,     -1,  0.0 },
{  NULL,  NULL, MAXINT,  32,    32,    1,   -1,      0,      0,     -1,  0.0 },
{  NULL,  NULL, MAXINT,  16,    16,    1,   -1,      0,      0,     -1,  0.0 },
{  NULL,  NULL, MAXINT,   4,     4,    1,   -1,      0,      0,     -1,  0.0 },
{  NULL,  NULL, MAXINT, 100,   100,    1,   -1,      0,      0,     -1,  0.0 },
{  NULL,  NULL, MAXINT,  40,    40,    1,   -1,      0,      0,     -1,  0.0 },
{  NULL,  NULL, MAXINT,  64,    64,    1,   -1,      0,      0,     -1,  0.0 },
{  NULL,  NULL, MAXINT,   4,     4, FSCALE, -1,      0,      0,     -1,  0.0 },
{  NULL,  NULL, MAXINT,   4,     4, FSCALE, -1,      0,      0,     -1,  0.0 },
{  NULL,  NULL, MAXINT,   4,     4, FSCALE, -1,      0,      0,     -1,  0.0 },
} ;

extern char *cmdstr[] ;        /* Command line options (for saving). */
extern char *mess[] ;          /* Messages. */
extern char *perf_res[] ;      /* Perfmeter resource strings. */
extern char *pmenu_help[] ;    /* Frame menu help values. */
extern char *pmenu_items[] ;   /* Frame menu string values. */
extern char *sstrs[] ;         /* Scales. */
extern char *ustrs[] ;         /* Usage message. */
extern char *vstrs[] ;         /* Various strings. */

int *getdata(), meter_selected() ;

Data d ;             /* Perfmeter static data. */
Vars v ;             /* Perfmeter variables and options. */


void
do_perfmeter(argc, argv)
int argc ;
char **argv ;
{
  char *ptr ;
  int havehost = FALSE ;
  int reshost = FALSE ;      /* True if remote host set via X resource. */
  int cmdmeter, i, j, pid ;
  struct meter *mp ;

  init_cmdline_opts() ;     /* Initialise command line text strings. */
  v->appname = NULL ;

  if ((ptr = strrchr(argv[0], '/')) != NULL)
    read_str(&v->appname, ptr+1) ;
  else read_str(&v->appname, argv[0]) ;

/*  Search through all the command line arguments, looking for -name.
 *  If it's present, then this name with be used, when looking for X resources
 *  for this application. When the rest of the command line arguments are
 *  checked later on, then the -name argument (if found) is ignored.
 */
 
  for (i = 0; i < argc; i++)
    if (EQUAL(argv[i], cmdstr[(int) O_NAME]))
      {
        if ((i+1) > argc) usage(v->appname) ;
        read_str(&v->appname, argv[i+1]) ;
        break ;
      }

  init_text() ;               /* Setup text strings depending upon language. */
  init_opt_vals() ;           /* Initialise options to default values. */
  load_resources() ;          /* Get resources from various places. */
  read_resources() ;          /* Read resources from merged database. */

  if (v->hostname != NULL) reshost = TRUE ;
  cmdmeter = 0 ;              /* No command line dial/charts yet. */
  argc-- ;
  argv++ ;
  while (argc > 0)
    {
      if (argv[0][0] == '-' || argv[0][0] == '+')
        {
          if (argv[0][2] != '\0') goto toolarg ;
          switch (argv[0][1])
            {
              case 'a' : for (i = 0 ; i < MAXMETERS ; i++)
                           v->meters[i].m_showing = i + 1 ;
                         v->dlen = MAXMETERS ;
                         argc++ ;
                         argv-- ;
                         break ;

              case 'c' : FPRINTF(stderr, mess[(int) M_CACHE]) ;
                         argc -= 2 ;
                         argv += 2 ;
                         break ;

              case 'd' : v->dtype = DIAL ;    /* Draw dials initially. */
                         argc++ ;
                         argv-- ;
                         break ;

              case 'g' : v->dtype = GRAPH ;   /* Draw graphs initially. */
                         argc++ ;
                         argv-- ;
                         break ;

              case 'l' : v->save_log = TRUE ;
                         argc++ ;
                         argv-- ;
                         break ;

              case 'h' : if (argc < 2) usage(v->appname) ;
                         v->hourhandintv = atoi(argv[1]) ;
                         break ;

              case 'm' : if (argc < 2) usage(v->appname) ;
                         v->minutehandintv = atoi(argv[1]) ;
                         break ;

              case 'n' : if (argc < 2) usage(v->appname) ;
                         read_str(&v->sname, argv[1]) ;
                         v->save_log = TRUE ;
                         break ;

              case 'p' : if (argc < 2) usage(v->appname) ;
                         v->pagelength = atoi(argv[1]) ;
                         if (v->pagelength <= 0) v->pagelength = PAGELENGTH ;
                         v->scount = v->pagelength ;
                         break ;

              case 's' : if (argc < 2) usage(v->appname) ;
                         v->sampletime = atoi(argv[1]) ;
                         break ;

              case 'v' : if (argc == 1 || argv[1] == NULL || argv[1][0] == '-')
                           usage(v->appname) ;
                         /* FALL THROUGH*/

              case 't' : if (argc < 2) usage(v->appname) ;
                         for (i = 0, mp = v->meters ; i < v->length ; i++, mp++)
                           if (strcmp(argv[1], mp->m_rname) == 0)
                             break ;
                         if (i >= v->length) usage(v->appname) ;

/*  A command line option has been given for a dial/chart. Clear any possible
 *  settings of dial/charts via X resources, if this is the first command
 *  line dial/chart setting.
 */
                         if (!cmdmeter)
                           {
                             cmdmeter = 1 ;
                             v->dlen = 0 ;
                             for (j = 0; j < MAXMETERS; j++)
                               v->meters[j].m_showing = 0 ;
                           }

                         if (mp->m_showing == 0)
                           {
                             v->dlen++ ;
                             mp->m_showing = v->dlen ;
                           }
                         v->visible = i ;
                         break ;

              case '?' : usage(v->appname) ;
                         break ;

              case 'C' : if (argc < 3) usage(v->appname) ;
                         for (i = 0, mp = v->meters ; i < v->length ; i++, mp++)                           if (strcmp(argv[1], mp->m_rname) == 0)
                             break ;
                         if (i >= v->length) usage(v->appname) ;

                         mp->m_ceiling = atoi(argv[2]) ;
                         if (mp->m_ceiling <= 0) mp->m_ceiling = -1 ;

                         argc-- ;
                         argv++ ;
                         break ;

              case 'D' : v->debug = TRUE ;            /* Debug flag. */
                         argc++ ;
                         argv-- ;
                         break ;

              case 'H' : v->direction = HORIZONTAL ;  /* Horizontal dir. */
                         argc++ ;
                         argv-- ;
                         break ;

              case 'L' : v->solid_graph = LINED ;     /* Line graph type. */
                         argc++ ;
                         argv-- ;
                         break ;

              case 'S' : v->solid_graph = SOLID ;     /* Solid graph type. */
                         argc++ ;
                         argv-- ;
                         break ;

              case 'V' : v->direction = VERTICAL ;    /* Vertical direction. */
                         argc++ ;
                         argv-- ;
                         break ;

              case 'M' : if (argc < 4) usage(v->appname) ;

/*  With OW V3, the -M option expects four additional parameters; the meter
 *  type followed by three max values. To be backward compatible with OW V2,
 *  it also has to just accept three numeric values.
 *
 *  If the first parameter after the -M is a valid meter name, then the next
 *  three values are the max values, and will affect that named meter,
 *  otherwise the three numerical values will affect the currently visible
 *  meter. If these options came from V2, then there should only be one visible
 *  meter.
 */
                         for (i = 0, mp = v->meters ; i < v->length ; i++, mp++)                           if (strcmp(argv[1], mp->m_rname) == 0)
                             break ;
                         if (i < v->length)
                           {
                             argc-- ;
                             argv++ ;
                           }
                         else mp = &(v->meters[v->visible]) ;
                         if (argv[1] != NULL &&
                             argv[1][0] != '-' && argv[1][0] != '+')
                           mp->m_curmax = atoi(argv[1]) ;
                         if (argv[2] != NULL &&
                             argv[2][0] != '-' && argv[2][0] != '+')
                           mp->m_minmax = atoi(argv[2]) ;
                         if (argv[3] != NULL &&
                             argv[3][0] != '-' && argv[3][0] != '+')
                           mp->m_maxmax = atoi(argv[3]) ;
                         argc -= 2 ;
                         argv += 2 ;
                         break ;

              default :
              toolarg :                /* Pick up generic tool arguments. */

                       if (EQUAL(argv[0], cmdstr[(int) O_NAME]))
                         {
                           if (argc < 2) usage(v->appname) ;
                           argc -= 2 ;
                           argv += 2 ;
                           continue ;
                         }

/*  Check to see if the user has supplied a -Wn or a +Wn argument. The first
 *  signifies no titleline required, and the latter indicates the opposite.
 */

                       if (!strncmp(&argv[0][1], "Wn", 2))
                         {
                           v->istitle = (argv[0][0] == '-') ? FALSE : TRUE ;
                           argc-- ;
                           argv++ ;
                           continue ;
                         }

                       usage(v->appname) ;
            }
          argc-- ;
          argv++ ;
        }
      else
        {
          if (havehost == TRUE) usage(v->appname) ;
          read_str(&v->hostname, argv[0]) ;
          havehost = TRUE ;
          v->remote = 1 ;
        }
      argc-- ;
      argv++ ;
    }

/*  If there is one of more graph which is monitoring a ceiling value, and
 *  no ceiling color set (via X resources), set it to red.
 */

  for (i = 0, mp = v->meters ; i < v->length ; i++, mp++)
    if (mp->m_ceiling >= 0)
      if (v->colstr[(int) C_CEILING] == NULL)
        {
          read_str(&v->colstr[(int) C_CEILING], "#FF0000") ;
          break ;
        }

  check_args() ;       /* Check for size, label, scale and position. */

/*  If the perfmeter X resource "showLocal" is set to false, and no remote
 *  host name  has been given, then reset to monitoring the local host.
 */

  if (v->remote && !reshost && !havehost) v->remote = 0 ;

  if (!v->dlen)      /* No graphs/dials set?  Just monitor CPU. */
    v->dlen = v->meters[(int) CPU].m_showing = 1 ;

  if (v->sampletime <= 0 || v->hourhandintv < 0 || v->minutehandintv < 0)
    usage(v->appname) ;

  v->shortexp = (int) ((1 - ((double) v->sampletime / max(v->hourhandintv,
                               v->sampletime))) * FSCALE) ;
  v->longexp  = (int) ((1 - ((double) v->sampletime / max(v->minutehandintv,
                               v->sampletime))) * FSCALE) ;
  make_canvas() ;
  make_xlib_stuff() ;           /* Create Xlib objects for doing graphics. */

  v->iconheight  = 48 + v->charheight ;
  v->numchars    = (ICONWIDTH - 4) / v->charwidth ;
  v->iconwidth   = 10 * v->charwidth ;
  v->riconheight = 60 + v->charheight ;

  adjust_frame_size() ;         /* Set perfmeter open frame dimensions. */
  pm_create_frame_menu() ;      /* Create open window popup menu. */
  start_events() ;              /* Assign event handlers. */
  set_frame_hints() ;           /* Setup min. and max. frame size hints. */

  v->maxsave = (v->direction == VERTICAL) ? v->iwidth
                                          : (v->iwidth / v->dlen) ;
 
/* Dynamically allocate save */
 
  v->save = (int *)
    LINT_CAST(calloc(1, (unsigned) (sizeof(int) * MAXMETERS * v->maxsave))) ;

  toggle_logfile() ;
  if (setup() < 0)              /* Setup timer. */
    {
      v->dead = 1 ;
      v->sick = 0 ;
      keeptrying() ;
    }

  updatedata() ;                         /* Get first set of data. */
  for (i = 0; i < v->length; i++)        /* Initialise arrays. */
    for (j = 1; j < v->maxsave; j++)
      SAVE_ACCESS(i, j) = -1 ;

  pid = getpid() ;
  SETPGRP(pid, pid) ;
  SIGNAL(SIGTERM, killkids) ;
  SIGNAL(SIGHUP,  SIG_IGN) ;

  write_cmdline() ;
  set_size();
  start_tool() ;          /* Set itimer event handler and display window. */
  killkids() ;
}


void
adjust_cache_size(width, height)
int width, height ;
{
  int i, j, min_size, offset, *old_save, old_size, size, start ;

  if (v->dtype == DIAL) return ;
  old_size = v->maxsave ;
  size     = (v->direction == VERTICAL) ? width : (width / v->dlen) ;
  if (old_size != size)
    {
      min_size   = (old_size < size) ? old_size : size ;
      old_save   = v->save ;
      v->maxsave = size ;
      v->save    = (int *)
        LINT_CAST(calloc(1, (unsigned) (sizeof(int) * MAXMETERS * size))) ;

/* Copy across old cache values, and fill the rest of the cache with -1. */

      if (old_save != NULL)
        {
          for (i = 0; i < v->length; i++)
            {
              offset = v->saveptr - min_size ; 
              if (offset < 0) offset = v->saveptr + old_size - min_size ;
              for (j = 0; j < min_size; j++)
                {
                  SAVE_ACCESS(i, j) = *(old_save + (i * old_size) + offset) ;
                  if (++offset == old_size) offset = 0 ;
                } 
            }
          FREE((char *) old_save) ;
          start = min_size ;
        } 
      else start = 0 ;

      for (i = 0; i < v->length; i++)        /* Initialise remainder to -1. */
        for (j = start; j < v->maxsave; j++)
          SAVE_ACCESS(i, j) = -1 ;

      v->saveptr = min_size - 1 ;
    }
}


char *
adjust_string(str, width)
char *str ;
int width ;
{
  static char s[MAXCHARS], chs[2] ;
  int i, len ;
  int x = 0 ;

  STRCPY(s, str) ;
  chs[1] = '\0' ;
  len = strlen(s) ;
  for (i = 0; i < len; i++)
    {
      chs[0] = s[i] ;
      x += get_str_width(chs) ;
      if (x > width)
        {
          s[i] = '\0' ;
          if (i > 0) s[i-1] = '-' ;
          break ;
        }
    }    
  return(s) ;
}


static int
check_log_exists()   /* Reopen log file if it no longer exists. */
{
  struct stat sbuf ;

  if (stat(v->sname, &sbuf) == -1) {
      if (v->sfp != NULL) FCLOSE(v->sfp) ;
      if ((v->sfp = fopen(v->sname, "a")) == NULL) {
          display_prompt(EBADLOG) ;
          v->save_log = FALSE ;
          item_set_val(ILOG, FALSE) ;
	  return 0;
        }
  }
  return 1;
}


void
do_char(val)             /* Handle keyboard accelerator option. */
int val ;
{
  enum area_type      old_dtype     = v->dtype ;
  enum direction_type old_direction = v->direction ;
  int                 olddlen       = v->dlen ;
  int                 old_remote    = v->remote ;
  int                 oldwidth      = v->width ;

  v->curch = val ;
  switch (val)
    {
      case CHAR_DIR       : item_set_val(IDIR, !item_get_val(IDIR)) ;
                            break ;

      case CHAR_GRAPHTYPE : item_set_val(ITYPE, !item_get_val(ITYPE)) ;
                            break ;

      case CHAR_MONTYPE   : if (v->hostname != NULL)
                              item_set_val(IMACH, !item_get_val(IMACH)) ;
                            break ;

      case CHAR_STYLE     : item_set_val(IGRAPH, !item_get_val(IGRAPH)) ;
                            v->solid_graph = (enum graph_type)
                                             item_get_val(IGRAPH) ;
                            break ;

      case CHAR_SAMPLE    : item_set_val(ILOG, !item_get_val(ILOG)) ;
                            v->save_log = item_get_val(ILOG) ;
                            toggle_logfile() ;
                            break ;

      case CHAR_QUIT      : exit(0) ;                    /* Quit perfmeter. */

      case '1'            :
      case '2'            :
      case '3'            :
      case '4'            :
      case '5'            :
      case '6'            :
      case '7'            :
      case '8'            :
      case '9'            : item_set_val(ISTIME, val - '0') ;
                            set_timer(val - '0') ;
                            break ;

      case CHAR_HRDEC     : if (v->hourhandintv > 0)     /* Hour hand dec. */
                              item_set_val(IHHAND, v->hourhandintv - 1) ;
                            break ;

      case CHAR_HRINC     :                              /* Hour hand inc. */
                            item_set_val(IHHAND, v->hourhandintv + 1) ;
                            break ;

      case CHAR_MINDEC    : if (v->minutehandintv > 0)   /* Minute hand dec. */
                              item_set_val(IMHAND, v->minutehandintv - 1) ;
                            break ;

      case CHAR_MININC    :                              /* Minute hand inc. */
                            item_set_val(IMHAND, v->minutehandintv + 1) ;
      default		  :
			   v->curch = '\0' ;
			   return; /* dont set props */	
    }

  set_prop_vals(item_get_str(IMACHNAME), old_dtype, old_direction,
                olddlen, old_remote, oldwidth) ;

  v->curch = '\0' ;        /* Clear keyboard accelerator value. */
}


/*  Repaint the entire meter. Paint dials or graphs depending upon dtype. */

void
do_paint()
{
  enum col_type color ;
  int h, i, j, left, offset, right, xoff, yoff, x, y ;
  struct meter *mp = &(v->meters[v->visible]) ;
  char host[MAXCHARS], name[MAXCHARS] ;
 
  xoff = XOFF() ;
  yoff = YOFF() ;

  if (v->dtype == DIAL)
    {
      draw_image(xoff, yoff, ICONWIDTH, ICONHEIGHT,
                 (enum col_type) v->visible, GSRC, RSPEEDO) ;

      draw_line(xoff, yoff + ICONHEIGHT,
                xoff, yoff + ICONHEIGHT + v->charheight,
                (enum col_type) v->visible) ;
      draw_line(xoff + 1, yoff + ICONHEIGHT,
                xoff + 1, yoff + ICONHEIGHT + v->charheight,
                (enum col_type) v->visible) ;

      draw_line(xoff + ICONWIDTH - 1, yoff + ICONHEIGHT,
                xoff + ICONWIDTH - 1, yoff + ICONHEIGHT + v->charheight,
                (enum col_type) v->visible) ;
      draw_line(xoff + ICONWIDTH - 2, yoff + ICONHEIGHT,
                xoff + ICONWIDTH - 2, yoff + ICONHEIGHT + v->charheight,
                (enum col_type) v->visible) ;

      draw_line(xoff,                 yoff + ICONHEIGHT + v->charheight,
                xoff + ICONWIDTH - 1, yoff + ICONHEIGHT + v->charheight,
                (enum col_type) v->visible) ;

      do_update() ;                 /* Draw dynamic portion */

      clear_area(xoff + 2, yoff + ICONHEIGHT, ICONWIDTH - 4, v->charheight) ;

      x = xoff + 2 ;
      SPRINTF(name, "%s%1d", mp->m_name, mp->m_curmax) ;
      if (get_str_width(name) > ICONWIDTH)
        {
          STRCPY(name, adjust_string(name, ICONWIDTH)) ;
          draw_text(x, yoff + ICONHEIGHT, GSRC, name) ;
        }
      else
        {
          SPRINTF(name, "%s", mp->m_name) ;
          draw_text(x, yoff + ICONHEIGHT, GSRC, name) ;

          SPRINTF(name, "%1d", mp->m_curmax) ;
          x = xoff + ICONWIDTH - get_str_width(name) - 2 ;
          draw_text(x, yoff + ICONHEIGHT, GSRC, name) ;
        }

      if ((v->direction == HORIZONTAL && v->remote) ||
          (v->direction == VERTICAL && v->remote && v->offset == (v->dlen-1)))
        {
          draw_line(xoff, yoff + ICONHEIGHT + v->charheight,
                    xoff, yoff + ICONHEIGHT + (2 * v->charheight),
                    (enum col_type) v->visible) ;
          draw_line(xoff + 1, yoff + ICONHEIGHT + v->charheight,
                    xoff + 1, yoff + ICONHEIGHT + (2 * v->charheight),
                    (enum col_type) v->visible) ;

          draw_line(xoff + ICONWIDTH - 1,
                    yoff + ICONHEIGHT + v->charheight,
                    xoff + ICONWIDTH - 1,
                    yoff + ICONHEIGHT + (2 * v->charheight),
                    (enum col_type) v->visible) ;
          draw_line(xoff + ICONWIDTH - 2,
                    yoff + ICONHEIGHT + v->charheight,
                    xoff + ICONWIDTH - 2,
                    yoff + ICONHEIGHT + (2 * v->charheight),
                    (enum col_type) v->visible) ;
          x = xoff + 2 ;
          STRCPY(host, adjust_string(v->hostname, ICONWIDTH)) ;
          offset = (ICONWIDTH - get_str_width(v->hostname) - 4) / 2 ;
          if (offset > 0) x += offset ;
          draw_text(x, yoff + ICONHEIGHT + v->charheight, GSRC, host) ;
        }
    }    
  else
    { 
      clear_area(xoff + BORDER, yoff + BORDER,
                 v->width, v->height + v->charheight) ;

      if (v->direction == HORIZONTAL) y = v->height + BORDER ;
      else y = (v->offset + 1) * v->height +
               (v->offset * v->charheight) + BORDER ;

      x = xoff + BORDER ;
      SPRINTF(name, "%s%1d", mp->m_name, mp->m_curmax) ;
      if (get_str_width(name) > v->width)
        {
          STRCPY(name, adjust_string(name, v->width)) ;
          draw_text(x, y, GSRC, name) ; 
        } 
      else
        {
          SPRINTF(name, "%s", mp->m_name) ;
          draw_text(x, y, GSRC, name) ;
 
          SPRINTF(name, "%1d", mp->m_curmax) ;
          x = xoff + BORDER + v->width - get_str_width(name) ;
          draw_text(x, y, GSRC, name) ; 
        }

      if (v->remote)
        {
          x = xoff + BORDER ;
          STRCPY(host, adjust_string(v->hostname, v->width)) ;
          offset = (v->width - get_str_width(v->hostname)) / 2 ;
          if (offset > 0) x += offset ;
          draw_text(x, (v->height + v->charheight) *
                    ((v->direction == HORIZONTAL) ? 1 : v->dlen) + BORDER,
                    GSRC, host) ;
        }

      if (v->direction == HORIZONTAL)
        {
          h = v->height + v->charheight + (2 * BORDER) ;
          if (v->remote) h += v->charheight ;
          if (v->offset != (v->dlen -1))
            draw_line(xoff + v->width + (GRAPHGAP / 2) + 2, yoff,
                      xoff + v->width + (GRAPHGAP / 2) + 2, yoff + h,
                      C_FOREGND) ;
        }

      right = scaletograph(SAVE_ACCESS(v->visible, v->saveptr)) ;
      if (right == -1) goto out ;
      mp->m_lastval = right ;
      if ((j = v->saveptr - 1) < 0) j = v->maxsave - 1 ;
      for (i = v->width - 1; (i > 0) && (j != v->saveptr); i--)
        {
          left = scaletograph(SAVE_ACCESS(v->visible, j)) ;
          if (left == -1) goto out ;
          if (v->solid_graph == LINED)
            draw_line(xoff + i - 1 + BORDER, yoff + left  + BORDER,
                      xoff + i     + BORDER, yoff + right + BORDER,
                      set_graph_col(v->visible, right)) ;
          else
            { 
              color = set_graph_col(v->visible, right) ;
              draw_line(xoff + i + BORDER, yoff + BORDER + right,
                        xoff + i + BORDER, yoff + BORDER + v->height - 1,
                        color) ;
              if (!v->ceiling_solid && color == C_CEILING)
                {
                  right = v->height - (v->height * mp->m_ceiling) /
                          mp->m_curmax ;
                  draw_line(xoff + i + BORDER, yoff + BORDER + right,
                            xoff + i + BORDER, yoff + BORDER + v->height - 1,
                            (enum col_type) v->visible) ;
                }
            }    
          right = left ;
          if (--j < 0) j = v->maxsave - 1 ;
        }
out:

      if (v->dead)
        dead_or_sick(xoff + BORDER, yoff + BORDER, v->width, v->height,
                     (enum col_type) v->visible, DEAD) ;

      if (v->sick)
        dead_or_sick(xoff + BORDER, yoff + BORDER, v->width, v->height,
                     (enum col_type) v->visible, SICK) ;
    }
}


/* Get the metered data from the host via RPC and process it to compute the
 * actual values (rates) that perfmeter wants to see.
 */
 
int *
getdata_time()
{
  register int i, msecs, t, x ;
  enum clnt_stat clnt_stat ;
  int intrs, maxtfer, ppersec, pag, spag, sum, swtchs ;
 
  clnt_stat = clnt_call(d->client, RSTATPROC_STATS, xdr_void, 0,
                        xdr_statstime, (caddr_t)&d->stats_time, TIMEOUT) ;
  if (clnt_stat == RPC_TIMEDOUT) return(NULL) ;
  if (clnt_stat != RPC_SUCCESS)
    {
      if (!v->sick)
        {
          clnt_perror(d->client, vstrs[(int) V_CLNTERR]) ;
          v->sick = 1 ;
        }
      meter_paint() ;
      return(NULL) ;
    }
  if (d->oldtime == (int *) NULL)         /* Allocate memory for structures. */
    {
      d->cpustates = 4 ;                  /* Compatibility with version 3. */
      d->dk_ndrive = 4 ;
      d->oldtime   = (int *) calloc(1, (d->cpustates * sizeof(int))) ;
      d->xfer1     = (int *) calloc(1, (d->dk_ndrive * sizeof(int))) ;
      if ((d->oldtime == NULL) || (d->xfer1 == NULL))
        {
          FPRINTF(stderr, vstrs[(int) V_MALLOC]) ;
          exit(1) ;
        }
    }    
  for (i = 0; i < d->cpustates; i++)
    {
      t = d->stats_time.cp_time[i] ;
      d->stats_time.cp_time[i] -= d->oldtime[i] ;
      d->oldtime[i] = t ;
    }
  
  /* Change for Mars -- only sum kernel and user  See 1105770 */
  /* Here's how it works now: */
  /*     0 = USER   */
  /*     1 = WAIT   */
  /*     2 = KERNEL */
  /*     3 = IDLE   */
  /*     NOTE - this is not how it works in <sys/sysinfo.h>.  That file      */
  /*     is how the info gets from the kernel to rpc.rstatd, not how it      */
  /*     gets from there to here.  (Go figure.) It had been that user,       */
  /*     wait, and kernel were added up to get how busy the CPU was, but     */
  /*     under Mars, machines (SS10's in particular) spend their free time   */
  /*     in wait, so perfmeter got pegged even whe your machine was waiting. */

  t = d->stats_time.cp_time[0]
    + d->stats_time.cp_time[2] ;
  x = d->stats_time.cp_time[0] 
    + d->stats_time.cp_time[1] 
      + d->stats_time.cp_time[2]
        + d->stats_time.cp_time[3] ;

  if (x <= 0)
    {
      t++ ;
      d->badcnt++ ;
      if (d->badcnt >= TRIES)
        {
          v->sick = 1 ;
          meter_paint() ;
        }
    }    
  else if (v->sick)
    {
      d->badcnt = v->sick = 0 ;
      meter_paint() ;
    }
  else d->badcnt = 0 ;

  d->ans[(int) CPU] = (int) ((100 * (double) t) / x);

  GETTIMEOFDAY(&d->tm, (struct timezone *) 0) ;
  msecs = (int) (1000 * ((double) d->tm.tv_sec  - d->oldtm.tv_sec) +
                  (d->tm.tv_usec - d->oldtm.tv_usec) / 1000) ;

/*  Bug #1013912 fix: in the rare occurence that msecs = 0,
 *  reset to msecs = 1 to ensure no "divide by zero" errors later.
 */

  if (msecs == 0) msecs = 1 ;

  sum      = d->stats_time.if_ipackets + d->stats_time.if_opackets ;
  ppersec  = (int) (1000 * ((double) sum - d->total) / msecs) ;
  d->total = sum ;
  d->ans[(int) PKTS] = ppersec ;

  d->ans[(int) COLL] = (int) (FSCALE *
         ((double) d->stats_time.if_collisions - d->totcoll) * 1000 / msecs) ;
  d->totcoll        = d->stats_time.if_collisions ;
  d->ans[(int) ERR] = (int) (FSCALE *
         ((double) d->stats_time.if_ierrors - d->toterr) * 1000 / msecs) ;
  d->toterr         = d->stats_time.if_ierrors ;

  if (!d->getdata_init_done)
    {
      pag    = 0 ;
      spag   = 0 ;
      intrs  = 0 ;
      swtchs = 0 ;
      d->getdata_init_done = 1 ;
    }
  else
    { 
      pag    = d->stats_time.v_pgpgout + d->stats_time.v_pgpgin - d->oldp ;
      pag    = 1000 * pag / msecs ;
      spag   = d->stats_time.v_pswpout + d->stats_time.v_pswpin - d->oldsp ;
      spag   = 1000 * spag / msecs ;
      intrs  = d->stats_time.v_intr - d->oldi ;
      intrs  = 1000 * intrs / msecs ;
      swtchs = d->stats_time.v_swtch - d->olds ;
      swtchs = 1000 * swtchs / msecs ;
    }
  d->oldp             = d->stats_time.v_pgpgin + d->stats_time.v_pgpgout ;
  d->oldsp            = d->stats_time.v_pswpin + d->stats_time.v_pswpout ;
  d->oldi             = d->stats_time.v_intr ;
  d->olds             = d->stats_time.v_swtch ;
  d->ans[(int) PAGE]  = pag ;
  d->ans[(int) SWAP]  = spag ;
  d->ans[(int) INTR]  = intrs ;
  d->ans[(int) CNTXT] = swtchs ;
  d->ans[(int) LOAD]  = d->stats_time.avenrun[0] ;
  for (i = 0; i < d->dk_ndrive; i++)
    {
      t = d->stats_time.dk_xfer[i] ;
      d->stats_time.dk_xfer[i] -= d->xfer1[i] ;
      d->xfer1[i] = t ;
    }

  maxtfer = 0 ;
  for (i = 0; i < d->dk_ndrive; i++)
     maxtfer += d->stats_time.dk_xfer[i] ;

  maxtfer            = (int) (1000 * (double) maxtfer) / msecs ;
  d->ans[(int) DISK] = maxtfer ;
  d->oldtm           = d->tm ;
  return(d->ans) ;
}


int *
getdata_var()
{
  register int i, msecs, t, x ;
  enum clnt_stat clnt_stat ;
  int intrs, maxtfer, pag, ppersec, spag, sum, swtchs ;

  if (d->oldtime == (int *) NULL)
    {
      d->stats_var.dk_xfer.dk_xfer_val = (int *) NULL ;
      d->stats_var.cp_time.cp_time_val = (int *) NULL ;
    }
  clnt_stat = clnt_call(d->client, RSTATPROC_STATS,
                        xdr_void, 0, xdr_statsvar, (caddr_t)&d->stats_var, TIMEOUT) ;
  if (clnt_stat == RPC_TIMEDOUT) return(NULL) ;
  if (clnt_stat != RPC_SUCCESS)
    {
      if (!v->sick)
        {
          clnt_perror(d->client, vstrs[(int) V_CLNTERR]) ;
          v->sick = 1 ;
        }
      meter_paint() ;
      return(NULL) ;
    }
  if (d->oldtime == (int * ) NULL)      /* Allocate memory for structures. */
    {
      d->cpustates = d->stats_var.cp_time.cp_time_len ;
      d->dk_ndrive = d->stats_var.dk_xfer.dk_xfer_len ;
      d->oldtime   = (int *) calloc(1, (d->cpustates * sizeof(int))) ;
      d->xfer1     = (int *) calloc(1, (d->dk_ndrive * sizeof(int))) ;
      if ((d->oldtime == NULL) || (d->xfer1 == NULL))
        {
          FPRINTF(stderr, vstrs[(int) V_MALLOC]) ;
          exit(1) ;
        }
    }    

  for (i = 0; i < d->cpustates; i++)
    {
      t = d->stats_var.cp_time.cp_time_val[i] ;
      d->stats_var.cp_time.cp_time_val[i] -= d->oldtime[i] ;
      d->oldtime[i] = t ;
    }
  
  /* change for Mars -- only sum kernel and user.  See 1105770  */
  t = d->stats_var.cp_time.cp_time_val[0] 
    + d->stats_var.cp_time.cp_time_val[2] ;
  x = d->stats_var.cp_time.cp_time_val[0] 
    + d->stats_var.cp_time.cp_time_val[1] 
      + d->stats_var.cp_time.cp_time_val[2]
	+ d->stats_var.cp_time.cp_time_val[3] ;

  if (x <= 0)
    {
      t++ ;
      d->badcnt++ ;
      if (d->badcnt >= TRIES)
        {
          v->sick = 1 ;
          meter_paint() ;
        }
    }    
  else if (v->sick)
    {
      d->badcnt = v->sick = 0 ;
      meter_paint() ;
    }
  else d->badcnt = 0 ;

  d->ans[(int) CPU] = (int) ((100 * (double) t) / x);

  GETTIMEOFDAY(&d->tm, (struct timezone *) 0) ;
  msecs = (int) (1000 * ((double) d->tm.tv_sec - d->oldtm.tv_sec) +
                  (d->tm.tv_usec - d->oldtm.tv_usec) / 1000) ;

/*  Bug #1013912 fix: in the rare occurence that msecs = 0,
 *  reset to msecs = 1 to ensure no "divide by zero" errors later.
 */

  if (msecs == 0) msecs = 1 ;

  sum      = d->stats_var.if_ipackets + d->stats_var.if_opackets ;
  ppersec  = (int) (1000 * ((double) sum - d->total) / msecs) ;
  d->total = sum ;
  d->ans[(int) PKTS] = ppersec ;

  d->ans[(int) COLL] = (int) (FSCALE *
         ((double) d->stats_var.if_collisions - d->totcoll) * 1000 / msecs) ;
  d->totcoll         = d->stats_var.if_collisions ;
  d->ans[(int) ERR]  = (int) (FSCALE *
         ((double) d->stats_var.if_ierrors - d->toterr) * 1000 / msecs) ;
  d->toterr          = d->stats_var.if_ierrors ;

  if (!d->getdata_init_done)
    {
      pag    = 0 ;
      spag   = 0 ;
      intrs  = 0 ;
      swtchs = 0 ;
      d->getdata_init_done = 1 ;
    }
  else
    { 
      pag    = d->stats_var.v_pgpgout + d->stats_var.v_pgpgin - d->oldp ;
      pag    = 1000 * pag / msecs ;
      spag   = d->stats_var.v_pswpout + d->stats_var.v_pswpin - d->oldsp ;
      spag   = 1000 * spag / msecs ;
      intrs  = d->stats_var.v_intr - d->oldi ;
      intrs  = 1000 * intrs / msecs ;
      swtchs = d->stats_var.v_swtch - d->olds ;
      swtchs = 1000 * swtchs / msecs ;
    }

  d->oldp             = d->stats_var.v_pgpgin + d->stats_var.v_pgpgout ;
  d->oldsp            = d->stats_var.v_pswpin + d->stats_var.v_pswpout ;
  d->oldi             = d->stats_var.v_intr ;
  d->olds             = d->stats_var.v_swtch ;
  d->ans[(int) PAGE]  = pag ;
  d->ans[(int) SWAP]  = spag ;
  d->ans[(int) INTR]  = intrs ;
  d->ans[(int) CNTXT] = swtchs ;
  d->ans[(int) LOAD]  = d->stats_var.avenrun[0] ;
  for (i = 0; i < d->dk_ndrive; i++)
    {
      t = d->stats_var.dk_xfer.dk_xfer_val[i] ;
      d->stats_var.dk_xfer.dk_xfer_val[i] -= d->xfer1[i] ;
      d->xfer1[i] = t ;
    }

  maxtfer = d->stats_var.dk_xfer.dk_xfer_val[0] ;
  for (i = 0; i < d->dk_ndrive; i++)
    maxtfer += d->stats_var.dk_xfer.dk_xfer_val[i] ;

  maxtfer            = (int) ((1000 * (double) maxtfer) / msecs) ;
  d->ans[(int) DISK] = maxtfer ;
  d->oldtm           = d->tm ;
  return(d->ans) ;
}


/* Convert raw data into properly scaled and averaged data and save it for
 * later redisplay.
 */

void
updatedata()
{
  int i, *dp, old, tmp ;
  struct meter *mp ;

  if (v->dead) return ;
  if (v->vers == RSTATVERS_VAR) dp = getdata_var() ;
  else                          dp = getdata_time() ;

  if (dp == NULL)
    {
      v->dead = 1 ;
      v->sick = 0 ;
      meter_paint() ;
      keeptrying() ;
      return ;
    }

/*  Don't have to worry about save[old] being -1 the very first time thru,
 *  because we are called before save is initialized to -1.
 */

  old = v->saveptr ;
  if (++v->saveptr == v->maxsave) v->saveptr = 0 ;
  for (i = 0, mp = v->meters ; i < v->length ; i++, mp++, dp++)
    {
      if (*dp < 0)          /* Should print out warning if this happens */
        *dp = 0 ;
      tmp = (v->longexp * SAVE_ACCESS(i, old) +
            (*dp * FSCALE)/mp->m_scale * (FSCALE - v->longexp)) >> FSHIFT ;
      if (tmp < 0)          /* Check for wraparound */
        tmp = mp->m_curmax * FSCALE ;
      SAVE_ACCESS(i, v->saveptr) = tmp ;
      tmp = (v->shortexp * mp->m_longave +
            (*dp * FSCALE/mp->m_scale) * (FSCALE - v->shortexp)) >> FSHIFT ;
      if (tmp < 0)          /* Check for wraparound */
        tmp = mp->m_curmax * FSCALE ;
      mp->m_longave = tmp ;
    }
}


void
meter_update()               /* Update display with current date points. */
{
  if (v->dlen > 1)
    {
      for (v->visible = 0 ; v->visible < MAXMETERS ; v->visible++)
        if (v->meters[v->visible].m_showing)
          {
            v->offset = v->meters[v->visible].m_showing - 1 ;
            do_update() ;
          }
    }
  else
    { 
      v->offset = 0 ;
      do_update() ;
    }
  if (v->save_log == TRUE)
    {
        if (check_log_exists()) {
      		if ((v->scount % v->pagelength) == 0) write_sample_header() ;
      		write_sample() ;
      		v->scount++ ;
	}
    }
}


int
XOFF()    /* Determine X value based on direction and current offset. */
{  
       if (v->direction == VERTICAL) return(0) ;
  else if (v->dtype == DIAL) return(ICONWIDTH * v->offset) ;
  else                       return((v->width + GRAPHGAP) * v->offset) ;
}
   
   
int
YOFF()    /* Determine Y value based on direction and current offset. */
{  
       if (v->direction == HORIZONTAL) return(0) ;
  else if (v->dtype == DIAL) return((ICONHEIGHT + v->charheight) * v->offset) ;
  else                       return((v->height + v->charheight) * v->offset) ;
}  
   
   
void
dead_or_sick(x, y, width, height, color, image)
int x, y, width, height ;
enum col_type color ;
enum image_type image ;
{  
  if (v->iscolor) clear_area(x, y, width, height) ;
  draw_image(x, y, width, height, color, (v->iscolor ? GSRC : GOR), image) ;
}


/* Update the display with the current data points.
 * If displaying dials, paint in a new set of hands, otherwise
 * scroll the graph to the left and add the new data point.
 */
 
void
do_update()
{
  int minutedata, hourdata, xoff, yoff ;
  struct meter *mp = &(v->meters[v->visible]) ;
  int curmax = mp->m_curmax ;
 
  xoff       = XOFF() ;
  yoff       = YOFF() ;
  minutedata = getminutedata() ;
  hourdata   = gethourdata() ;
  mp->m_val  = curmax * ((double) minutedata / (curmax * FSCALE)) ;
   
/* If data is off scale, increase max, up to limit of maxmax. */
   
  if (!mp->m_resizing)
    {
      if (minutedata > curmax * FSCALE)
        {
          while (curmax * FSCALE < minutedata && 2*curmax <= mp->m_maxmax)
            curmax *= 2 ;
          mp->m_curmax = curmax ;
          mp->m_resizing = 1 ;
          do_paint() ;
          mp->m_resizing = 0 ;
          return ;
        }
    }    

/*  If all data on graph is under 1/3 of max, decrease max, to limit
 *  of minmax.
 */

  if (!mp->m_resizing)
    {
      if (minutedata < curmax * FSCALE / 3) mp->m_undercnt++ ;
      else                                  mp->m_undercnt = 0 ;
      if (mp->m_undercnt >= v->width)
        {
          if (curmax / 2 >= mp->m_minmax)
            {
              mp->m_undercnt = 0 ;
              mp->m_curmax /= 2 ;
              mp->m_resizing = 1 ;
              do_paint() ;
              mp->m_resizing = 0 ;
              return ;
            }
        }
    }    

  if (v->dtype == DIAL)
    {
      struct hands *hand ;
      int trunc, datatometer ;

      draw_image(xoff, yoff, ICONWIDTH, ICONHEIGHT,
                 (enum col_type) v->visible, GSRC, RSPEEDO) ;
      datatometer = curmax * FSCALE / 30 ;

/* Short hand displays short average */

      trunc = ((hourdata > curmax * FSCALE) ? curmax * FSCALE : hourdata) ;
      hand = &hand_points[(trunc / datatometer + 45) % 60] ;
      draw_line(xoff + hand->x1,     yoff + hand->y1,
                xoff + hand->hour_x, yoff + hand->hour_y,
                set_dial_col(v->visible)) ;
      draw_line(xoff + hand->x2,     yoff + hand->y2,
                xoff + hand->hour_x, yoff + hand->hour_y,
                set_dial_col(v->visible)) ;

/* Long hand displays long average */

      trunc = ((minutedata > curmax*FSCALE) ? curmax*FSCALE : minutedata) ;
      hand = &hand_points[(trunc / datatometer + 45) % 60] ;
      draw_line(xoff + hand->x1,    yoff + hand->y1,
                xoff + hand->min_x, yoff + hand->min_y,
                set_dial_col(v->visible)) ;
      draw_line(xoff + hand->x2,    yoff + hand->y2,
                xoff + hand->min_x, yoff + hand->min_y,
                set_dial_col(v->visible)) ;

      if (v->dead)
        dead_or_sick(xoff, yoff, ICONWIDTH, ICONHEIGHT,
                     (enum col_type) v->visible, DEAD) ;

      if (v->sick)
        dead_or_sick(xoff, yoff, ICONWIDTH, ICONHEIGHT,
                     (enum col_type) v->visible, SICK) ;
    }
  else
    { 
      enum col_type color ;
      int graphdata, val ;

      if (v->dead || v->sick) return ;
      if (!v->width && !v->height) return ;

      graphdata = scaletograph(minutedata) ;
      if (mp->m_lastval == -1) mp->m_lastval = graphdata ;
      shift_left(xoff + BORDER, yoff + BORDER, v->width, v->height) ;
      clear_area(xoff + v->width - 1 + BORDER, yoff + BORDER, 1, v->height) ;
      if (v->solid_graph == LINED)
        draw_line(xoff + v->width - 2 + BORDER, yoff + BORDER + mp->m_lastval,
                  xoff + v->width - 1 + BORDER, yoff + graphdata + BORDER,
                  set_graph_col(v->visible, graphdata)) ;
      else
        { 
          color = set_graph_col(v->visible, graphdata) ;
          draw_line(xoff + v->width - 1 + BORDER, yoff + BORDER + graphdata,
                    xoff + v->width - 1 + BORDER, yoff + BORDER + v->height - 1,                    color) ;
          if (!v->ceiling_solid && color == C_CEILING)
            {
              val = (v->height-1) - ((v->height-1) * mp->m_ceiling) /
                    mp->m_curmax ;
              draw_line(xoff + v->width - 1 + BORDER, yoff + BORDER + val,
                        xoff + v->width - 1 + BORDER,
                        yoff + BORDER + v->height - 1,
                        (enum col_type) v->visible) ;
            }
        }
      mp->m_lastval = graphdata ;
    }
}


/* Scale data value to graph value. */

int
scaletograph(x)
int x ;
{

  if (x == -1) return(-1) ;        /* To detect end of data */
  if (x > v->meters[v->visible].m_curmax*FSCALE) return(0) ;
  else                              /* Should round */
    return((v->height-1) - (x*(v->height-1)) /
           (FSCALE * v->meters[v->visible].m_curmax)) ;
}


enum col_type
set_graph_col(n, data)
int n, data ;
{
  double val ;

  if (v->meters[n].m_ceiling == -1) return((enum col_type) n) ;
  val = ((double) (v->height-1) - data) *
        v->meters[n].m_curmax / (v->height-1) ;
  if (val > (double) v->meters[n].m_ceiling) return(C_CEILING) ;
  return((enum col_type) n) ;
}
             
             
enum col_type
set_dial_col(n)
int n ;
{
  if (v->meters[n].m_ceiling == -1) return((enum col_type) n) ;
  if (v->meters[n].m_val > (double) v->meters[n].m_ceiling) return(C_CEILING) ;
  return((enum col_type) n) ;
}


int
getminutedata()
{
  return(SAVE_ACCESS(v->visible, v->saveptr)) ;
}
 
 
int
gethourdata()
{
  return(v->meters[v->visible].m_longave) ;
}


char *
item_get_str(itype)      /* Get property item string value. */
enum item_type itype ;
{
	char *val;

  	if (v->isprops == TRUE) 
		if ((val = xv_item_get_str(itype)) != NULL && *val != NULL) 
    			return(val) ;

  	switch (itype)
    	{
      	case IMACHNAME : return(v->hostname) ;
      	case ILOGNAME  : return(v->sname) ;
    	}
/*NOTREACHED*/
}


int
item_get_val(itype)    /* Get property item integer value. */
enum item_type itype ;
{
  int i, itrackval ;

  if (v->isprops == TRUE)
    return(xv_item_get_val(itype)) ;

  switch (itype)
    {
      case ITRACK  : itrackval = 0 ;
                     for (i = MAXMETERS-1; i >= 0; i--)
                       itrackval = (itrackval << 1) +
                                   (v->meters[i].m_showing != 0) ;
                     return(itrackval) ;
      case IDIR    : return((int) v->direction) ;
      case ITYPE   : return((int) v->dtype) ;
      case IGRAPH  : return((int) v->solid_graph) ;
      case IMACH   : return(v->remote) ;
      case ISTIME  : return(v->sampletime) ;
      case IHHAND  : return(v->hourhandintv) ;
      case ILOG    : return(v->save_log) ;
      case IMHAND  : return(v->minutehandintv) ;
    }
/*NOTREACHED*/
}


void
item_set_val(itype, value)    /* Set property item integer value. */
enum item_type itype ;
int value ;
{
  int i, n ;

  if (v->isprops == TRUE)
    {
      xv_item_set_val(itype, value) ;
      return ;
    }

  switch (itype)
    {
      case IDIR   : v->direction      = (enum direction_type) value ;
                    break ;
      case ITYPE  : v->dtype          = (enum area_type) value ;
                    break ;
      case IGRAPH : v->solid_graph    = (enum graph_type) value ;
                    break ;
      case IMACH  : v->remote         = value ;
                    break ;
      case ISTIME : v->sampletime     = value ;
                    break ;
      case IHHAND : v->hourhandintv   = value ;
                    break ;
      case ILOG   : v->save_log       = value ;
                    break ;
      case IMHAND : v->minutehandintv = value ;
    }
}


void
write_cmdline()
{
  int argc, i, j ;
  char *argv[150], buf[255] ;
 
  argc = 0 ;
  for (i = 0; i < 150; i++) argv[i] = NULL ;
  if (v->dlen == MAXMETERS) ADD_CMD(O_ALL) ;
  else if (v->dlen > 1)
    {
      for (i = 0; i < MAXMETERS; i++)
        for (j = 0; j < MAXMETERS; j++)
          if (v->meters[j].m_showing == i+1)
            {
              ADD_CMD(O_TYPE) ;
              ADD_CMDS(v->meters[j].m_rname) ;
            }
    }
  else
    {
      ADD_CMD(O_TYPE) ;
      ADD_CMDS(v->meters[v->visible].m_rname) ;
    }
 
  for (i = 0 ; i < MAXMETERS ; i++)
    if (v->meters[i].m_showing)
      {
        ADD_CMD(O_MAXS) ;
        ADD_CMDS(v->meters[i].m_rname) ;
        ADD_CMDN(v->meters[i].m_curmax) ;
        ADD_CMDN(v->meters[i].m_minmax) ;
        ADD_CMDN(v->meters[i].m_maxmax) ;
 
        if (v->meters[i].m_ceiling != -1)
          {
            ADD_CMD(O_LIM) ;
            ADD_CMDS(v->meters[i].m_rname) ;
            ADD_CMDN(v->meters[i].m_ceiling) ;
          }
      }

  if (v->dtype == DIAL) ADD_CMD(O_DIAL) ;     /* Display type. */
  else                  ADD_CMD(O_GRAPH) ;

  if (v->dtype == GRAPH)                      /* Graph type (line/solid). */
    if (v->solid_graph == SOLID) ADD_CMD(O_SOLID) ;
    else                         ADD_CMD(O_LINED) ;

  if (v->direction == HORIZONTAL) ADD_CMD(O_HORIZ) ;   /* Direction. */
  else                            ADD_CMD(O_VERT) ;

  if (v->istitle == TRUE) ADD_CMD(O_HASLABEL) ;    /* Frame titleline? */
  else                    ADD_CMD(O_NOLABEL) ;

  if (v->save_log == TRUE)                        /* Samples being logged? */
    {
      ADD_CMD(O_LOG) ;
      if (v->sname != NULL && v->sname[0] != '\0')
        {
          ADD_CMD(O_LOGNAME) ;
          ADD_CMDS(v->sname) ;
        }
      ADD_CMD(O_PAGELEN) ;
      ADD_CMDN(v->pagelength) ;
    }

  ADD_CMD(O_SAMPLE) ;
  ADD_CMDN(v->sampletime) ;

  ADD_CMD(O_HRINTV) ;
  ADD_CMDN(v->hourhandintv) ;

  ADD_CMD(O_MININTV) ;
  ADD_CMDN(v->minutehandintv) ;

  if (v->remote && v->hostname != NULL)
    read_str(&argv[argc++], v->hostname) ;

  save_cmdline(argc, argv) ;

  for (i = 0; i < argc; i++)                    /* Free up memory. */
    if (argv[i] != NULL)
      {
        FREE(argv[i]) ;
        argv[i] = NULL ;
      }
}


void
meter_paint()                     /* Paint appropriate meters. */
{
  clear_area(0, 0, v->winwidth, v->winheight) ;

  if (v->dlen > 1)
    {
      for (v->visible = 0; v->visible < MAXMETERS; v->visible++)
        if (v->meters[v->visible].m_showing)
          {
            v->offset = v->meters[v->visible].m_showing - 1 ;
            do_paint() ;
          }
    }
  else
    {
      v->offset = 0 ;
      do_paint() ;
    }
}


void
repaint_from_cache(x, y, width, height)
int x, y, width, height ;
{
  int off, xval, paint = 1;

  if (v->dlen > 1)
    {
      for (v->visible = 0; v->visible < MAXMETERS; v->visible++)
        if (v->meters[v->visible].m_showing)
          {

            v->offset = v->meters[v->visible].m_showing - 1 ;
            if (v->direction == HORIZONTAL)
              {
                off = XOFF() ; 
                if (x > (off + v->width + GRAPHGAP)) continue ;
                if ((x + width) < off) paint = 0 ;
		else                   paint = 1 ;
                xval = x - XOFF() ;
              }
            else
              {
                off = YOFF() ;
                if (y > (off + v->height + BORDER + v->charheight)) continue ;
                if ((y + height) < off) paint = 0 ; 
		else                    paint = 1 ;
                xval = x ;
              }
	    if (paint)
	      {
                if (width > 1 || v->dtype == DIAL) do_paint() ;
		else                               repaint_line(xval) ;
	      }
	}
  }
  else
    { 
      v->offset = 0 ;
      if (width > 1 || v->dtype == DIAL) do_paint() ;
      else                               repaint_line(x) ;
    }
}


void
repaint_line(x)
int x ;
{
  enum col_type color ;
  struct meter *mp = &(v->meters[v->visible]) ;
  int index, left, off, right, xoff, yoff ;

  xoff = XOFF() ;
  yoff = YOFF() ;

  index = v->width - x + BORDER ;
  off = v->saveptr - index ;
  if (off < 0) off = v->saveptr + v->maxsave - index ;
  right = scaletograph(SAVE_ACCESS(v->visible, off)) ;
  if (right == -1) return ;

  off-- ;
  if (off < 0) off = v->maxsave - 1 ;
  left = scaletograph(SAVE_ACCESS(v->visible, off)) ;
  if (left == -1) return ;

  if (v->solid_graph == LINED)
    draw_line(xoff + x - 2, yoff + left  + BORDER,
              xoff + x - 1, yoff + right + BORDER,
              set_graph_col(v->visible, right)) ;
  else
    {
      color = set_graph_col(v->visible, right) ;
      draw_line(xoff + x - 1, yoff + BORDER + right,
                xoff + x - 1, yoff + BORDER + v->height - 1, color) ;
      if (!v->ceiling_solid && color == C_CEILING)
        {
          right = v->height - (v->height * mp->m_ceiling) /
                  mp->m_curmax ;
          draw_line(xoff + x - 1, yoff + BORDER + right,
                    xoff + x - 1, yoff + BORDER + v->height - 1,
                    (enum col_type) v->visible) ;
        }
    }
}


void
write_sample()
{
  struct tm *tm ;
  char date[18], host[MAXSTRING] ;
  int clock, i ;

  clock = time((time_t *) 0) ;
  tm    = localtime((time_t *) &clock) ;
  SPRINTF(date, "%02d/%02d/%02d %02d:%02d:%02d",
                tm->tm_mon+1, tm->tm_mday, (tm->tm_year % 100),
                tm->tm_hour, tm->tm_min,   tm->tm_sec) ;
  FPRINTF(v->sfp, "%s ", date) ;

  if (v->remote && v->hostname != NULL) STRCPY(host, v->hostname) ;
	else                                  STRCPY(host, vstrs[(int) V_LHOST]) ; 
/**  else                                  STRCPY(host, "localhost"); **/
  FPRINTF(v->sfp, "%s ", host) ;

  for (i = 0; i < MAXMETERS; i++)
    if (v->meters[i].m_showing)
      FPRINTF(v->sfp, "\t%s=%.2lf", v->meters[i].m_name, v->meters[i].m_val) ;
  FPUTS("\n", v->sfp) ;
  FFLUSH(v->sfp) ;
}


void
write_sample_header()
{
  FPUTS("\f\n", v->sfp) ;
  FFLUSH(v->sfp) ;
  v->scount = 0 ;
}


void
usage(name)
char *name ;
{
  int i ;
  
/*
 *  Usage: perfmeter [-a] [-d] -[g] [-h hourhandintv]
 *          [-l] [-m minutehandintv] [-name app-name]
 *          [-n samplefile] [-p pagelength] [-s sampletime]
 *          [-t value] [-v] [-C value ceiling] [-M value minmax maxmax]
 *          [-H] [-L] [-S] [-V] [-Wn] [+Wn] [-?] [hostname]
 *
 *      value is one of:
 *              cpu
 *              pkts
 *              page
 *              swap
 *              intr
 *              disk
 *              cntxt
 *              load
 *              colls
 *              errs
 *
 *  Keyboard accelerators:
 *      d - toggle direction (horizontal/vertical).
 *      g - toggle graph type (dial/graph).
 *      h - decrement hour hand interval by 1.
 *      H - increment hour hand interval by 1.
 *      m - decrement minute hand interval by 1.
 *      M - increment minute hand interval by 1.
 *      q - quit perfmeter.
 *      s - toggle graph style (solid/lined).
 *      S - toggle saving of samples to file.
 *      t - toggle monitoring mode (local/remote).
 *      1-9 - set sampletime to a range from 1 to 9 seconds.
 */

  if (isupper(name[0])) name[0] = tolower(name[0]) ;
  FPRINTF(stderr, ustrs[0], name, PATCHLEVEL) ;
  FPRINTF(stderr, ustrs[1], name) ;
  for (i = 2 ; i < 7; i++)
    FPUTS(ustrs[i], stderr) ;

  for (i = 0 ; i < v->length ; i++)
    FPRINTF(stderr, vstrs[(int) V_USAGE], v->meters[i].m_rname) ;

  for (i = 7 ; i < MAXUSAGE; i++)
    FPUTS(ustrs[i], stderr) ;

  exit(1) ;
}


/* SIGCHLD signal catcher.
 * Harvest any child (from keeptrying).
 * If can now contact host, request redisplay.
 */

void
ondeath()
{
#ifdef SVR4
  int status ;

  while (waitpid(-1, &status, WNOHANG) > 0)
#else
  union wait status ;

  while (wait3(&status, WNOHANG, (struct rusage *) 0) > 0)
#endif /*SVR4*/
    continue ;

  if (setup() < 0) keeptrying() ;
  else
    {
      v->dead = 0 ;

/* Can't redisplay from interrupt level, so set flag and do a notify_stop. */

      v->want_redisplay = 1 ;
      notify_stop() ;
    }
}


void
get_font_scale(s)
char *s ;
{
       if (!strcmp(s, sstrs[(int) S_SMALL]))      v->fontsize = 10 ;
  else if (!strcmp(s, sstrs[(int) S_MEDIUM]))     v->fontsize = 12 ;
  else if (!strcmp(s, sstrs[(int) S_LARGE]))      v->fontsize = 14 ;
  else if (!strcmp(s, sstrs[(int) S_EXTRALARGE])) v->fontsize = 19 ;
}


void
init_opt_vals()                   /* Initialise options to default values. */
{
  int i ;

  v->iheight = -1 ;               /* Initial height of perfmeter window. */
  v->iwidth = -1 ;                /* Initial width of perfmeter window. */
  v->length = sizeof(meters_init) / sizeof(meters_init[0]) ;
  v->solid_graph   = LINED ;      /* Graph type (line or solid). */
  v->ceiling_solid = FALSE ;      /* Don't draw ceiling lines in solid. */
  v->collect_data  = TRUE ;       /* Turn off perfmeter if obscured. */
  v->resize_graph  = TRUE ;       /* Resize for change in # of graphs. */
  v->dead       = NULL ;
  v->dtype      = GRAPH ;         /* Display type (dials or graphs). */
  v->direction  = VERTICAL ;      /* Direction for dials/graphs. */
  v->debug      = FALSE ;         /* No debugging by default. */
  v->hasicon    = FALSE ;         /* Use standard perfmeter icon by default. */
  v->isprops    = FALSE ;         /* No property sheet initially. */
  v->istitle    = FALSE ;         /* No frame titleline present. */
  v->hostname   = NULL ;          /* No remote hostname initially. */
  v->sname      = NULL ;          /* No default sample filename. */
  v->titleline  = NULL ;          /* No title line. */
  v->visible    = 0 ;             /* Default to first meter. */
  v->fontname   = NULL ;          /* User supplied font name. */
  v->fontsize   = 10 ;             /* Small font for graph labling. */
  v->curch      = '\0' ;          /* No keyboard accelerator value. */
  v->pagelength = PAGELENGTH ;    /* Default sample file page length. */
  v->scount     = PAGELENGTH ;    /* Force a page header. */
  v->save_defs  = FALSE ;         /* Don't save defaults. */
  v->save_log   = 0 ;             /* Don't save samples to file by default. */
  v->saveptr    = 0 ;             /* Initial pointer into cyclic cache. */
  v->sfp        = NULL ;          /* No initial sample file. */
  v->vers       = VER_NONE ;

  read_str(&v->iconlabel, vstrs[(int) V_ILABEL]) ;  /* Default icon label. */

  for (i = 0; i < MAXMETERS+1; i++) v->colstr[i] = NULL ;

/* Dynamically allocate and initialize meters */

  v->meters = (struct meter *)
              LINT_CAST(calloc(1, (unsigned) (sizeof(meters_init)))) ;
  for (i = 0; i < MAXMETERS; i++) v->meters[i] = meters_init[i] ;
  v->dlen = 0 ;
  v->remote = 0 ;
  v->sampletime = 2 ;
  v->minutehandintv = 2 ;
  v->hourhandintv = 20 ;
  v->oldsocket = -1 ;
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
  if (!strcmp(tempstr, vstrs[(int) V_TRUE])) *boolval = TRUE ;
  else                                       *boolval = FALSE ;
  return(1) ;
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


int
setup()         /* Initialize the connection to the metered host. */
{
  enum clnt_stat clnt_stat ;
  struct hostent *hp ;
  struct timeval timeout ;
  struct sockaddr_in serveradr ;
  int snum ;

  snum = RPC_ANYSOCK ;
  MEMSET((char *) &serveradr, 0, sizeof(serveradr)) ;
  if (v->remote == TRUE && v->hostname != NULL)
    {
      if ((hp = gethostbyname(v->hostname)) == NULL)
        {
          display_prompt(ENOTFOUND) ;
          v->remote = FALSE ;
          item_set_val(IMACH, 0) ;     /* Reset to local machine. */
          setup() ;
          return(0) ;
        }
      MEMCPY((char *) &serveradr.sin_addr, hp->h_addr, hp->h_length) ;
      endhostent();
    }
  else
    {
     if ((hp = gethostbyname("localhost")) != NULL)
        MEMCPY((char *) &serveradr.sin_addr, hp->h_addr, hp->h_length) ;
      else
        serveradr.sin_addr.s_addr = inet_addr(vstrs[(int) V_IADDR]) ;
      endhostent();
    }

  serveradr.sin_family = AF_INET ;
  serveradr.sin_port = 0 ;
  timeout.tv_sec = 5 ;
  timeout.tv_usec = 0 ;

  if ((v->vers == VER_NONE) || (v->vers == RSTATVERS_VAR))
    {
      if ((d->client = clntudp_bufcreate(&serveradr, RSTATPROG,
                      RSTATVERS_VAR, timeout, &snum,
                      sizeof(struct rpc_msg), UDPMSGSIZE)) == NULL)
        return (-1) ;
 
      clnt_stat = clnt_call(d->client, NULLPROC,
                            xdr_void, 0,xdr_void, 0, TIMEOUT) ;
           if (clnt_stat == RPC_SUCCESS) v->vers = RSTATVERS_VAR ;
      else if (clnt_stat != RPC_PROGVERSMISMATCH)
        {
          clnt_destroy(d->client) ;
          return(-1) ;
        }
      else
        {                             /* Version mismatch. */
          clnt_destroy(d->client) ;
          v->vers = RSTATVERS_TIME ;
        }       
    }               

  if ((v->vers == VER_NONE) || (v->vers == RSTATVERS_TIME))
    {
      snum = RPC_ANYSOCK ;
      if ((d->client = clntudp_bufcreate(&serveradr, RSTATPROG,
                      RSTATVERS_TIME, timeout, &snum,
                      sizeof(struct rpc_msg), sizeof(struct rpc_msg) +
                      sizeof(struct statstime)))== NULL)
        return (-1) ;
      clnt_stat = clnt_call(d->client, NULLPROC,
                            xdr_void, 0, xdr_void, 0, TIMEOUT) ;
      if (clnt_stat == RPC_SUCCESS) v->vers = RSTATVERS_TIME ;
      else
        {
          clnt_destroy(d->client) ;
          return (-1) ;
        }
    }

  if (v->oldsocket >= 0) CLOSE(v->oldsocket) ;
  v->oldsocket = snum ;
  if (d->oldclient) clnt_destroy(d->oldclient) ;
  d->oldclient = d->client ;
  return(0) ;
}


/* Fork a separate process to keep trying to contact the host
 * so that the main process can continue to service window
 * requests (repaint, move, stretch, etc.).
 */

void
keeptrying()
{  
#ifdef SVR4
  if ((int) sigset(SIGCHLD, ondeath) == -1) perror(vstrs[(int) V_SIGNAL]) ;
#else
  if ((int) signal(SIGCHLD, ondeath) == -1) perror(vstrs[(int) V_SIGNAL]) ;
#endif

  v->child_pid = fork() ;
  if (v->child_pid < 0)
    {
      perror(vstrs[(int) V_FORK]) ;
      exit(1) ;
    }
  if (v->child_pid == 0)
    for (;;)
      {
        sleep(1) ;
        if (setup() < 0) continue ;
        exit(0) ;
      }
}


/* Kill any background processes we may've started. */

void
killkids()
{
  SIGNAL(SIGCHLD, SIG_IGN) ;
  if (v->child_pid)
    KILL(v->child_pid, SIGTERM) ;    /* Get rid of forked processes */
  exit(0) ;
}


void
read_str(str, value)
char **str, *value ;
{
  if (*str != NULL) FREE(*str) ;
  if (value != NULL && strlen(value))
    {
      *str = (char *) calloc(1, (unsigned) (strlen(value) + 1)) ;
      STRCPY(*str, value) ;
    }
  else *str = NULL ;
}


void
read_resources()      /* Read all possible resources from server. */
{
  struct meter *mp ;
  int curmax, minmax, maxmax ;
  int boolval, i, intval, n ;
  char *ptr, str[MAXSTRING] ;

  if (get_bool_resource(R_DGRAPH, &boolval))
    v->dtype = (boolval) ? GRAPH : DIAL ;

  if (get_bool_resource(R_CSTYLE, &boolval))
    v->ceiling_solid = (boolval) ? TRUE : FALSE ;

  if (get_bool_resource(R_OBSCURE, &boolval))
    v->collect_data = (boolval) ? TRUE : FALSE ;

  if (get_bool_resource(R_RESIZE, &boolval))
    v->resize_graph = (boolval) ? TRUE : FALSE ;

  if (get_int_resource(R_HRINT,  &intval)) v->hourhandintv   = intval ;
  if (get_int_resource(R_MININT, &intval)) v->minutehandintv = intval ;
  if (get_int_resource(R_STIME,  &intval)) v->sampletime     = intval ;

  if (get_str_resource(R_MON, str))
    {
      n = 1 ;
      if ((ptr = strtok(str, ", \t")) != NULL)
        do
          {
            for (i = 0, mp = v->meters ; i < v->length ; i++, mp++)
              if (mp->m_rname && !strcmp(ptr, mp->m_rname))
                {
                  if (!mp->m_showing)
                    {
                      mp->m_showing = n ;
                      n++ ;
                    }
                  v->visible = i ;
                  v->dlen++ ;
                  break ;
                }
            ptr = strtok((char *) NULL, ", \t") ;
          }
        while (ptr) ;
    }

  if (get_str_resource(R_MACHINE, str)) read_str(&v->hostname, str) ;

  if (get_bool_resource(R_LOG,    &boolval)) v->save_log = boolval ;
  if (get_str_resource(R_LOGNAME, str))      read_str(&v->sname, str) ;
  if (get_str_resource(R_LFONT,   str))      read_str(&v->fontname, str) ;
  if (get_int_resource(R_PAGELEN, &intval))
    {
      v->pagelength = intval ;
      if (v->pagelength <= 0) v->pagelength = PAGELENGTH ;
      v->scount = v->pagelength ;
    }

  if (get_bool_resource(R_DVERT, &boolval))
    v->direction = (boolval) ? VERTICAL : HORIZONTAL ;

  if (get_bool_resource(R_GSOLID, &boolval))
    v->solid_graph = (boolval) ? SOLID : LINED ;

  for (i = 0, mp = v->meters ; i < v->length ; i++, mp++)
    if (get_str_resource((enum res_type) ((int) R_CPUMAX + i), str))
      if (sscanf(str, "%d %d %d", &curmax, &minmax, &maxmax) == 3)
        {
          mp->m_curmax = curmax ;
          mp->m_minmax = minmax ;
          mp->m_maxmax = maxmax ;
        }

  if (get_bool_resource(R_LOCAL,  &boolval)) v->remote  = !boolval ;
  if (get_bool_resource(R_TITLE,  &boolval)) v->istitle = boolval ;

  for (i = 0; i < MAXMETERS; i++)
    if (get_str_resource((enum res_type) ((int) R_CPUCOL + i), str))
      read_str(&v->colstr[(int) C_CPUCOL + i],   str) ;

  if (get_str_resource(R_CEILCOL, str))
      read_str(&v->colstr[(int) C_CEILING], str) ;

  for (i = 0; i < MAXMETERS; i++)
    if (get_int_resource((enum res_type) ((int) R_CPULIM + i), &intval) &&
        intval > 0)
      v->meters[(int) CPU + i].m_ceiling = intval ;
}


void
set_dial_dims(w, h, size)
int *w, *h, size ;
{
  if (v->direction == HORIZONTAL)
    {
      *w = ICONWIDTH * size + 2 ;
      *h = ICONHEIGHT + v->charheight + 2 ;
    }
  else
    { 
      *w = ICONWIDTH + 2 ;
      *h = (ICONHEIGHT + v->charheight) * size + 2 ;
    }
  if (v->remote) *h += v->charheight ;
}


void
adjust_track_val(itrackval)
int itrackval ;
{
  int i, j, n, oldshow[MAXMETERS], value ;

/*  Determine which of the previous set of graphs/dials the user still wants
 *  to see, and append the new ones to the right or below, depending upon the
 *  current display direction.
 */

  for (i = 0; i < MAXMETERS; i++)
    oldshow[i] = v->meters[i].m_showing ;

  value = itrackval ;
  for (i = 0; i < MAXMETERS; i++)
    {
      v->meters[i].m_showing = value & 1 ;
      value >>= 1 ;
    }

  for (i = 0; i < MAXMETERS; i++)
    if (oldshow[i] && !v->meters[i].m_showing) oldshow[i] = 0 ;

  n = 0 ;
  for (i = 0; i < MAXMETERS; i++)
    for (j = 0; j < MAXMETERS; j++)
      if (oldshow[j] == i+1)
        {
          n++ ;
          oldshow[j] = n ;
        }

  for (i = 0; i < MAXMETERS; i++)
    if (v->meters[i].m_showing && !oldshow[i])
      {
        n++ ;
        oldshow[i] = n ;
      }

  v->dlen = 0 ;
  for (i = 0; i < MAXMETERS; i++)
    {
      v->meters[i].m_showing = oldshow[i] ;
      if (v->meters[i].m_showing)
        {
          v->visible = i ;
          v->dlen++ ;
        }
    }
  if (v->isprops == TRUE) xv_item_set_val(ITRACK, itrackval) ;
}


/* Set properties for currently monitored machine. */

void
set_prop_vals(mname, old_dtype, old_direction, olddlen, old_remote, oldwidth)
char *mname ;
enum area_type old_dtype ;
enum direction_type old_direction ;
int olddlen, old_remote, oldwidth ;
{
  struct meter *oldmeters ;
  char *ptr , tempstr[MAXPATHLEN] ;
  int doresize      = FALSE ;
  int lname_changed = FALSE ;
  int reinit        = FALSE ;
  int i, itrackval, j, oldval ;

  v->dtype     = (enum area_type)      item_get_val(ITYPE) ;
  v->direction = (enum direction_type) item_get_val(IDIR) ;
  v->remote    = item_get_val(IMACH) ;
  v->solid_graph = (enum graph_type) item_get_val(IGRAPH) ;

/*  Check that the user hasn't toggled off all the monitoring options. If this
 *  is the case, then put the CPU one back on.
 */
 
  v->dlen = 0 ;
  if (!(itrackval = item_get_val(ITRACK)))
    itrackval = (1 << (int) CPU) ;

  adjust_track_val(itrackval) ;

/*  If the remote toggle is set, but no machine name is specified 
 *  switch back to local.  -welch
 */

  if (v->remote) 
	if (mname == NULL || mname[0] == '\0') {
      		v->remote = FALSE;
      		item_set_val(IMACH, 0) ;     /* Reset to local machine. */
      		mname = NULL;
    	}

  set_frame_hints() ;

  if (olddlen != v->dlen || old_dtype != v->dtype ||
      old_direction != v->direction || old_remote != v->remote)
    doresize = TRUE ;

  if (v->remote != old_remote)
    {
      v->remote = item_get_val(IMACH) ;   /* Changing remote to local host */
      if (v->remote)
        {
          if (!v->hostname || !mname || !EQUAL(v->hostname, mname))
            {
              if (v->hostname != NULL) FREE(v->hostname) ;
              if (mname != NULL && *mname != NULL)
                {
                  v->hostname = (char *)
                                calloc(1, (unsigned) (strlen(mname) + 1)) ;
                  STRCPY(v->hostname, mname) ;
                }
              else v->remote = FALSE ;
            }
        }
      reinit = TRUE ;
    }
  else if (v->remote == TRUE && v->hostname != NULL &&
           mname != NULL && strcmp(v->hostname, mname))
    {

/* Changing from one remote host to another */

      if (mname != NULL && strlen(mname))
        {
          if (v->hostname != NULL) FREE(v->hostname) ;
          v->hostname = (char *) calloc(1, (unsigned) (strlen(mname) + 1)) ;
          STRCPY(v->hostname, mname) ;
        }
      else v->remote = FALSE ;
      reinit = TRUE ;
    }

  ptr = item_get_str(ILOGNAME) ;
  if (ptr != NULL && *ptr != '\0')
    {
      if (v->sname == NULL || !EQUAL(v->sname, ptr)) lname_changed = TRUE ;
      read_str(&v->sname, ptr) ;
    }

  if (v->save_log != item_get_val(ILOG) || lname_changed == TRUE)
    {
      v->save_log = item_get_val(ILOG) ;
      toggle_logfile() ;
    }

  if (v->sampletime != item_get_val(ISTIME))
    {
      oldval = v->sampletime ;
      v->sampletime = item_get_val(ISTIME) ;
      if (v->sampletime <= 0 || v->sampletime > MAXSAMPLETIME)
        {
          v->sampletime = oldval ;
          item_set_val(ISTIME, v->sampletime) ;
        }
    }
  if (v->hourhandintv != item_get_val(IHHAND))
    {
      oldval = v->hourhandintv ;
      v->hourhandintv = item_get_val(IHHAND) ;
      if (v->hourhandintv <= 0 || v->hourhandintv > MAXINTERVAL)
        {
          v->hourhandintv = oldval ;
          item_set_val(IHHAND, v->hourhandintv) ;
        }
    }
  if (v->minutehandintv != item_get_val(IMHAND))
    {
      oldval = v->minutehandintv ;
      v->minutehandintv = item_get_val(IMHAND) ;
      if (v->minutehandintv <= 0 || v->minutehandintv > MAXINTERVAL)
        {
          v->minutehandintv = oldval ;
          item_set_val(IMHAND, v->minutehandintv) ;
        }
    }

  v->shortexp = (1 - ((double) v->sampletime / max(v->hourhandintv,
                               v->sampletime))) * FSCALE ;
  v->longexp  = (1 - ((double) v->sampletime / max(v->minutehandintv,
                               v->sampletime))) * FSCALE ;

  if (reinit)
    {
      v->dead = 0 ;
      if (v->remote == TRUE && v->hostname != NULL)
        {
        if (gethostbyname(v->hostname) == NULL)
          {
            display_prompt(ENOTFOUND) ;
            v->remote = FALSE ;
            item_set_val(IMACH, 0) ;     /* Reset to local machine. */
            return ;
          }
        endhostent();
        }

      SIGNAL(SIGCHLD, SIG_IGN) ;
      if (v->child_pid)
        KILL(v->child_pid, SIGTERM) ;    /* get rid of forked processes */

/* See if the required host exists. */

      if (setup() < 0)
        {
          v->dead = 1 ;
          keeptrying() ;
        }

/* Dynamically allocate save */

      FREE((char *) v->save) ;
      v->save = (int *)
        LINT_CAST(calloc(1, (unsigned) (sizeof(int) * MAXMETERS*v->maxsave))) ;

/* Dynamically allocate and initialize meters */

      oldmeters = v->meters ;
      v->meters = (struct meter *)
        LINT_CAST(calloc(1, (unsigned) (sizeof(struct meter) * MAXMETERS))) ;
      for (i = 0; i < MAXMETERS; i++) v->meters[i] = meters_init[i] ;
      for (i = 0; i < MAXMETERS; i++)
        {
          v->meters[i].m_showing = oldmeters[i].m_showing ;
          v->meters[i].m_ceiling = oldmeters[i].m_ceiling ;
        }
      FREE((char *) oldmeters) ;

/* Get first set of data, then initialize arrays. */

      FREE((char *) d->oldtime) ;
      d->oldtime = NULL ;
      FREE((char *) d->xfer1) ;
      d->xfer1 = NULL ;
      updatedata() ;
      for (i = 0 ; i < v->length ; i++)
        for (j = 1 ; j < v->maxsave ; j++) SAVE_ACCESS(i, j) = -1 ;
    }
  v->width = oldwidth ;
  if (doresize == TRUE)
    resize_display(v->dtype, olddlen, v->dlen, old_direction, v->direction) ;
  meter_paint() ;
  write_cmdline() ;
}


void
set_dims(w, h)   /* Set dimensions for window graph display. */
int w, h ;
{
  v->width = w - 2*BORDER - 1 ;
  v->height = h - 2*BORDER - 1 ;
  switch (v->direction)
    {
      case HORIZONTAL : v->width -= GRAPHGAP * (v->dlen - 1) ;
                        v->width /= v->dlen ;
                        v->height -= v->charheight ;
                        if (v->remote) v->height -= v->charheight ;
                        break ;
      case VERTICAL   : v->height -= v->charheight * v->dlen ;
                        if (v->remote) v->height -= v->charheight ;
                        v->height /= v->dlen ;
    }
}


void
toggle_logfile()
{
  char exname[MAXPATHLEN], name[MAXPATHLEN], *ptr ;

  if (v->save_log)
    {
      if (v->sfp != NULL) FCLOSE(v->sfp) ;
      ptr = item_get_str(ILOGNAME) ;
      if (ptr == NULL || *ptr == '\0') {
        SPRINTF(name, "%s/perfmeter.log%d", getenv("HOME"), getpid()) ;
	xv_item_set_str(ILOGNAME, name);
      }
      else STRCPY(name, ptr) ;

      ds_expand_pathname(name, exname) ;
      if (v->sname != NULL) FREE(v->sname) ;
      v->sname = (char *) calloc(1, (unsigned int) (strlen(exname) + 1)) ;
      STRCPY(v->sname, exname) ;

      if ((v->sfp = fopen(v->sname, "a")) == NULL)
        {
          display_prompt(EBADLOG) ;
          v->save_log = FALSE ;
          item_set_val(ILOG, FALSE) ;
        }
      else write_sample_header() ;
      pm_log_choice_proc(0, 1, 0);
    } 
  else { 
	if (v->sfp != NULL) FCLOSE(v->sfp) ;
	pm_log_choice_proc(0, 0, 0);
  }
}


char *
set_bool(value)
int value ;
{
  return((value) ? vstrs[(int) V_TRUE] : vstrs[(int) V_FALSE]) ;
}


void
write_resources()
{
  char intval[10] ;          /* For converting integer value. */
  char strval[MAXSTRING] ;   /* For converting the list of monitored meters. */
  char *value ;
  int i, j ;

  load_deskset_defs() ;

  put_resource(R_DGRAPH,  set_bool(v->dtype == GRAPH)) ;
  put_resource(R_CSTYLE,  set_bool(v->ceiling_solid == TRUE)) ;
  put_resource(R_OBSCURE, set_bool(v->collect_data == TRUE)) ;
  put_resource(R_RESIZE,  set_bool(v->resize_graph == TRUE)) ;
  put_resource(R_DVERT,   set_bool(v->direction == VERTICAL)) ;
  put_resource(R_GSOLID,  set_bool(v->solid_graph == SOLID)) ;
  put_resource(R_LOCAL,   set_bool(v->remote == FALSE)) ;
  put_resource(R_TITLE,   set_bool(v->istitle == TRUE)) ;
  put_resource(R_LOG,     set_bool(v->save_log == TRUE)) ;

  SPRINTF(intval, "%d", v->hourhandintv) ;
  put_resource(R_HRINT, intval) ;

  SPRINTF(intval, "%d", v->minutehandintv) ;
  put_resource(R_MININT, intval) ;

  SPRINTF(intval, "%d", v->sampletime) ;
  put_resource(R_STIME, intval) ;

  strval[0] = '\0' ;
  for (i = 0; i < MAXMETERS; i++)
    for (j = 0; j < MAXMETERS; j++)
      if (v->meters[j].m_showing == i+1)
        {
          STRCAT(strval, v->meters[j].m_rname) ;
          STRCAT(strval, ",") ;
        } 
  strval[strlen(strval)-1] = '\0' ;
  put_resource(R_MON, strval) ;

  if (v->hostname != NULL) put_resource(R_MACHINE, v->hostname) ;
  if (v->sname    != NULL) put_resource(R_LOGNAME, v->sname) ;
  if (v->fontname != NULL) put_resource(R_LFONT,   v->fontname) ;

  SPRINTF(intval, "%d", v->pagelength) ;
  put_resource(R_PAGELEN, intval) ;

  for (i = 0; i < MAXMETERS; i++)
    if (v->colstr[(int) C_CPUCOL + i] != NULL)
      put_resource((enum res_type) ((int) R_CPUCOL + i),
                   v->colstr[(int) C_CPUCOL + i]) ;

  if (v->colstr[(int) C_CEILING] != NULL)
    put_resource(R_CEILCOL, v->colstr[(int) C_CEILING]) ;

  for (i = 0; i < MAXMETERS; i++)
    if (v->meters[(int) CPU + i].m_ceiling != -1)
      {
        SPRINTF(intval, "%d", v->meters[(int) CPU + i].m_ceiling) ;
        put_resource((enum res_type) ((int) R_CPULIM + i), intval) ;
      }

  for (i = 0 ; i < MAXMETERS ; i++)
    if (v->meters[i].m_showing)
      {
        SPRINTF(strval, "%d %d %d",    v->meters[i].m_curmax,
                v->meters[i].m_minmax, v->meters[i].m_maxmax) ;
        put_resource((enum res_type) ((int) R_CPUMAX + i), strval) ;
      }

  save_resources() ;
}
