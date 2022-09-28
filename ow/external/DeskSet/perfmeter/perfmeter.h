/*
 *  @(#)perfmeter.h 1.11 93/12/07
 *
 *  Copyright (c) 1987-1992 Sun Microsystems, Inc.
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
#include <string.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>

#include <locale.h>

#ifdef XGETTEXT 
#define MSGFILE_ERROR    "SUNW_DESKSET_PERFMETER_ERR"
#define MSGFILE_LABEL    "SUNW_DESKSET_PERFMETER_LABEL"
#define MSGFILE_MESSAGE  "SUNW_DESKSET_PERFMETER_MSG"
#else
extern char *MSGFILE_ERROR;
extern char *MSGFILE_LABEL;
extern char *MSGFILE_MESSAGE;
#endif

#define  DGET(s)       (s)
#define  EGET(s)       (char *) dgettext(MSGFILE_ERROR,   s)
#define  LGET(s)       (char *) dgettext(MSGFILE_LABEL,   s)
#define  MGET(s)       (char *) dgettext(MSGFILE_MESSAGE, s)

/*  For all function declarations, if ANSI then use a prototype. */
 
#if  defined(__STDC__)
#define P(args)  args
#else  /* ! __STDC__ */
#define P(args)  ()
#endif  /* STDC */

#define  CLOSE         (void) close
#define  FCLOSE        (void) fclose
#define  FFLUSH        (void) fflush
#define  FPRINTF       (void) fprintf
#define  FPUTS         (void) fputs
#define  FREE          (void) free
#define  GETHOSTNAME   (void) gethostname
#define  GETTIMEOFDAY  (void) gettimeofday
#define  KILL          (void) kill

#ifndef SVR4
#define  KILLPG        (void) killpg
#endif

#define  MEMCPY        (void) memcpy
#define  MEMSET        (void) memset
#ifdef SVR4
#define  SETPGRP       (void) setpgid
#else
#define  SETPGRP       (void) setpgrp
#endif /*SRV4*/
#define  SIGNAL        (void) signal
#define  SPRINTF       (void) sprintf
#define  STRCAT        (void) strcat
#define  STRCPY        (void) strcpy

#define  BORDER             3     /* Border around graph area */

#define  EQUAL(a, b)        !strncmp(a, b, strlen(b))

#define  GRAPHGAP           10    /* Gap between graphs in horizontal dir. */
#define  ICONHEIGHT         36    /* Height of the speedometer dials. */
#define  ICONWIDTH          64    /* Width of the speedometer dials. */
#define  JUSTIFY_COL1       13    /* Property sheet column 1 justification. */
#define  JUSTIFY_COL2       52    /* Property sheet column 2 justification. */

#define  LARGE_POFFSET      50    /* Large panel item construction offset. */
#define  SMALL_POFFSET      5     /* Small panel item construction offset. */

#ifndef  MAX
#define  MAX(a, b)          (((a) > (b)) ? (a) : (b))
#endif

#define  MAXCHARS           240   /* Max number of chars in a meter */
#define  MAXINT             0x7fffffff
#define  MAXINTERVAL        3600  /* Maximum interval for hour and second. */
#define  MAXMETERS          10    /* Maximum number of graphs/dials. */
#define  MAXSAMPLETIME      3600  /* Max sample time in seconds (1hr). */
#define  MAXSCALES          4     /* Maximum number of predefined scales. */
#define  MAXSTRING          80    /* Maximum length of character strings. */
#define  MAXUSAGE           19    /* Maximum number of usage lines. */
#define  MINGRAPH           40    /* Minimum size of a single graph. */
#define  PAGELENGTH         60    /* Default sample file page length. */

#define  SAVE_ACCESS(i, data_point)  *(v->save+((i)*v->maxsave)+(data_point))

#define  TRIES              8     /* Number of bad rstat's before giving up */
#define  VER_NONE           -1

/*  Macros to improve understandability when writing away the user defined
 *  command line options.
 */

#define  ADD_CMD(s)    read_str(&argv[argc++], cmdstr[(int) s])
#define  ADD_CMDN(n)   SPRINTF(buf, "%d", n) ;   read_str(&argv[argc++], buf)
#define  ADD_CMDS(str) SPRINTF(buf, "%s", str) ; read_str(&argv[argc++], buf)

#define  max(x, y)          MAX((x), (y))
#define  min(x, y)          MIN((x), (y))

#ifndef  LINT_CAST
#ifdef   lint
#define  LINT_CAST(arg)    (arg ? 0 : 0)
#else
#define  LINT_CAST(arg)    (arg)
#endif /*lint*/
#endif /*LINT_CAST*/

/* Keyboard accelerators for perfmeter. */
 
#define  CHAR_DIR          'd'    /* Toggle direction (horizontal/vertical). */
#define  CHAR_GRAPHTYPE    'g'    /* Toggle graph type (dial/graph). */
#define  CHAR_HRDEC        'h'    /* Decrement hour hand interval by 1. */
#define  CHAR_HRINC        'H'    /* Increment hour hand interval by 1. */
#define  CHAR_MINDEC       'm'    /* Decrement minute hand interval by 1. */
#define  CHAR_MININC       'M'    /* Increment minute hand interval by 1. */
#define  CHAR_MONTYPE      't'    /* Toggle monitoring mode (local/remote). */
#define  CHAR_QUIT         'q'    /* Quit perfmeter. */
#define  CHAR_STYLE        's'    /* Toggle graph style (solid/lined). */
#define  CHAR_SAMPLE       'S'    /* Toggle saving of samples to a file. */

extern char *getenv P((const char *)) ;
extern char *strtok P((char *, const char *)) ;

enum area_type  { DIAL, GRAPH } ;                /* Drawable types. */

enum col_type { C_CPUCOL,  C_PKTSCOL, C_PAGECOL,  C_SWAPCOL,   /* Colors. */
                C_INTRCOL, C_DISKCOL, C_CNTXTCOL, C_LOADCOL,
                C_COLLCOL, C_ERRCOL,  C_CEILING,  C_FOREGND } ;

enum direction_type { HORIZONTAL, VERTICAL } ;   /* Drawing directions. */

enum error_type { ENOTFOUND, EBADLOG } ;         /* Possible alert errors. */

enum graph_type { LINED, SOLID } ;               /* Graph types. */

enum help_type { H_MCANVAS, H_MFRAME,    H_PROPS,      H_PFRAME,
                 H_VCHOICE, H_DIRCHOICE, H_DISPCHOICE, H_GCHOICE,
                 H_RCHOICE, H_MNAME,     H_SLEN,       H_HRHAND,
                 H_DOLOG,   H_MINHAND,   H_APPLY,      H_RESET,
                 H_LNAME,   H_DEFS } ;

enum image_type { RSPEEDO, DEAD, SICK } ;           /* Ropable images. */

/* Panel item types. */
enum item_type { ITRACK,    IDIR,   ITYPE,  IMACH, IGRAPH,
                 IMACHNAME, ISTIME, IHHAND, ILOG,  IMHAND, ILOGNAME } ;

enum mess_type { M_RSTAT, M_FONT, M_FRAME, M_CACHE, M_ICON } ;  /* Messages. */

enum meter_type { CPU, PKTS, PAGE, SWAP, INTR, DISK, CNTXT, LOAD, COLL, ERR } ;

enum not_type { N_NOTFOUND, N_CONT, N_BADLOG } ;

enum op_type    { GOR, GSRC } ;              /* Rasterop types. */

                                             /* Frame properties (get/set). */
/* Command line options. */
enum opt_type     { O_ALL,     O_TYPE,     O_DIAL,
                    O_GRAPH,   O_SOLID,    O_LINED,    O_HORIZ,
                    O_VERT,    O_HASLABEL, O_NOLABEL,
                    O_MAXS,    O_SAMPLE,   O_HRINTV,   O_MININTV,
                    O_LIM,     O_LOG,      O_LOGNAME,  O_PAGELEN,
                    O_NAME, } ;

enum prop_type  { PCMDLINE, PHEIGHT, PWIDTH } ;

/* Perfmeter resources. */

enum res_type   { R_DGRAPH,   R_HRINT,    R_MININT,   R_RESIZE,
                  R_STIME,    R_MON,      R_MACHINE,  R_DVERT,
                  R_GSOLID,   R_CPUMAX,   R_PKTSMAX,  R_PAGEMAX,
                  R_SWAPMAX,  R_INTRMAX,  R_DISKMAX,  R_CNTXTMAX,
                  R_LOADMAX,  R_COLLMAX,  R_ERRMAX,   R_LOCAL,
                  R_TITLE,    R_LOG,      R_LOGNAME,  R_PAGELEN,
                  R_CPUCOL,   R_PKTSCOL,  R_PAGECOL,  R_SWAPCOL,
                  R_INTRCOL,  R_DISKCOL,  R_CNTXTCOL, R_LOADCOL,
                  R_COLLCOL,  R_ERRCOL,   R_CPULIM,   R_PKTSLIM,
                  R_PAGELIM,  R_SWAPLIM,  R_INTRLIM,  R_DISKLIM,
                  R_CNTXTLIM, R_LOADLIM,  R_COLLLIM,  R_ERRLIM,
                  R_CEILCOL,  R_LFONT,    R_CSTYLE,   R_OBSCURE } ;

enum scale_type   { S_SMALL, S_MEDIUM, S_LARGE, S_EXTRALARGE } ;  /* Scales. */

enum var_type     { V_USAGE,  V_TRUE,    V_FALSE,                /* Various. */
                    V_LHOST,  V_IADDR,   V_SIGNAL,
                    V_FORK,   V_CLNTERR, V_PROPS,   V_FNAME,
                    V_FLABEL, V_ILABEL,  V_MTITLE,  V_MALLOC } ;

/* Structure describing value to be measured. */

struct  meter {
  char *m_name ;        /* Name of quantity (localised). */
  char *m_rname ;       /* Name of quantity (for cmdline comparisons. */
  int  m_maxmax ;       /* Maximum value of max */
  int  m_minmax ;       /* Minimum value of max */
  int  m_curmax ;       /* Current value of max */
  int  m_scale ;        /* Scale factor */
  int  m_lastval ;      /* Last value drawn */
  int  m_showing ;      /* Set if this meter to be displayed */
  int  m_resizing ;     /* Set if dial/graph max value changing. */
  int  m_ceiling ;      /* Ceiling limit. Show in red above this value. */
  int  m_longave ;      /* Long (hour hand) average */
  int  m_undercnt ;     /* Count of times under max */
  double m_val ;        /* Actual value (for saving samples and ceiling). */
} ;

struct perfStaticData    {          /* Perfmeter static data variables. */
  CLIENT *client ;
  CLIENT *oldclient ;
  statstime stats_time ;
  statsvar stats_var ;
  int cpustates ;
  int dk_ndrive ;
  int *oldtime ;
  int total ;                       /* Default to zero */
  int toterr ;                      /* Default to zero */
  int totcoll ;                     /* Default to zero */
  struct timeval tm ;
  struct timeval oldtm ;
  int *xfer1 ;
  int badcnt ;                      /* Default to zero */
  int ans[MAXMETERS] ;
  int oldi ;
  int olds ;
  int oldp ;
  int oldsp ;
  int getdata_init_done ;           /* Default to zero */
} PerfStaticData ;
 
typedef struct perfStaticData *Data ;

struct perfVars {              /* Perfmeter variables and options. */
  int child_pid ;
  int *save ;                  /* Saved values for redisplay; dynamically */
  int saveptr ;                /* Where I am in save+(visible*MAXMETERS) */
  int iscolor ;                /* Set if this is a color screen. */
  int dead ;                   /* Is remote machine dead? */
  int sick ;                   /* Is remote machine sick? */
  int dlen ;                   /* Number of meters to display */
  int debug ;                  /* Set if debugging is on. */
  int iheight ;                /* Initial height of perfmeter window. */
  int iwidth ;                 /* Initial width of perfmeter window. */
  int visible ;                /* Which quantity is visible*/
  int collect_data ;           /* Cache data, if perfmeter is obscured? */
  int offset ;                 /* Display offset for current meter */
  int oldsocket ;
  int length ;
  int remote ;                 /* Is meter remote? */
  int resize_graph ;           /* Resize window for change in # of graphs? */
  int vers ;                   /* RPC version number. */
  int want_redisplay ;         /* Flag set by interrupt handler */
  int width ;                  /* Current width of each graph area */
  int height ;                 /* Current height of each graph area */

  char *appname ;              /* Application name to use for resources. */
  char *colstr[MAXMETERS+1] ;  /* Possible X resource color names. */
  char curch ;                 /* Current keyboard accelerator value. */
  char *fontname ;             /* User supplied font name. */
  char *hostname ;             /* Name of host being metered */
  char *sname ;                /* Name of the sample filename. */
  char *iconlabel ;            /* The perfmeter icon label (if any). */
  char *titleline ;            /* Value of frame titleline (if present). */
 
  int sampletime ;             /* Sample seconds */
  int minutehandintv ;         /* Average this second interval */
  int hourhandintv ;           /* Long average over this seconds */
  int shortexp ;
  int longexp ;
  int meter_client ;
  int ceiling_solid ;          /* Draw ceiling lines in solid? */
  int charheight ;             /* Height of text for graph labling. */
  int charwidth ;              /* Width in pixels of one character. */
  int fontsize ;               /* Font scale for graph labels. */
  int hasicon ;                /* Set for icon name on command line. */
  int iconheight ;             /* Height of icon, dflt height of open tool */ 
  int iconwidth ;              /* Width of icon, dflt width of open tool */
  int isprops ;                /* Set if the props sheet has been created. */
  int istitle ;                /* Set if perfmeter frame has a title. */
  int maxsave ;                /* Number of values cached. */
  int numchars ;               /* Characters in an icon. */
  int pagelength ;             /* Page length in sample file. */
  int riconheight ;            /* Height for monitoring remote machine. */
  int save_defs ;              /* Indicates if X resources should be saved. */
  int save_log ;               /* Indicates if samples saved to file. */
  int scount ;                 /* No. of samples output (for page breaks). */
  int winheight ;              /* Full open window height. */
  int winwidth ;               /* Full open window width. */

  FILE *sfp ;                       /* File descriptor for sample file. */
  struct meter *meters ;            /* [MAXMETERS] */
  enum area_type dtype ;            /* Display type (dials or graphs). */
  enum direction_type direction ;   /* Direction for dials/graphs. */
  enum graph_type solid_graph ;     /* Graph type (line or solid). */
} PerfVars ;

typedef struct perfVars *Vars ;

char *adjust_string         P((char *, int)) ;
char *get_resource          P((enum res_type)) ;
char *item_get_str          P((enum item_type)) ;
char *set_bool              P((int)) ;
char *xv_item_get_str       P((enum item_type)) ;

enum col_type set_dial_col  P((int)) ;
enum col_type set_graph_col P((int, int)) ;

int item_get_val            P((enum item_type)) ;
int get_bool_resource       P((enum res_type, int *)) ;
int get_int_resource        P((enum res_type, int *)) ;
int get_str_resource        P((enum res_type, char *)) ;
int get_str_width           P((char *)) ;
int *getdata_time           P(()) ;        /* Version 3 (RSTATVERS_TIME) */
int *getdata_var            P(()) ;        /* Version 4 (RSTATVERS_VAR) */
int gethourdata             P(()) ;
int getminutedata           P(()) ;
int main                    P((int, char **)) ;
int scaletograph            P((int)) ;
int setup                   P(()) ;
int xv_item_get_val         P((enum item_type)) ;
int XOFF                    P(()) ;
int YOFF                    P(()) ;

void adjust_cache_size      P((int, int)) ;
void adjust_frame_size      P(()) ;
void adjust_track_val       P((int)) ;
void check_args             P(()) ;
static int check_log_exists       P(()) ;
void clear_area             P((int, int, int, int)) ;
void dead_or_sick           P((int, int, int, int,
                               enum col_type, enum image_type)) ;
void display_prompt         P((enum error_type)) ;
void do_char                P((int)) ;
void do_paint               P(()) ;
void do_perfmeter           P((int, char **)) ;
void do_update              P(()) ;
void draw_image             P((int, int, int, int, enum col_type,
                               enum op_type, enum image_type)) ;
void draw_line              P((int, int, int, int, enum col_type)) ;
void draw_text              P((int, int, enum op_type, char *)) ;
void get_font               P((int)) ;
void get_font_scale         P((char *)) ;
void get_geometry_size      P((char *, int *, int *)) ;
void init_binds             P((int *, char **)) ;
void init_cmdline_opts      P(()) ;
void init_opt_vals          P(()) ;
void init_text              P(()) ;
void item_set_val           P((enum item_type, int)) ;
void keeptrying             P(()) ;
void killkids               P(()) ;
void load_deskset_defs      P(()) ;
void load_resources         P(()) ;
void make_canvas            P(()) ;
void make_frame             P((int, char **)) ;
void make_image             P((enum image_type, int, int, unsigned short *)) ;
void make_xlib_stuff        P(()) ;
void meter_paint            P(()) ;
void meter_update           P(()) ;
void ondeath                P(()) ;
void pm_create_frame_menu   P(()) ;
void pm_create_props        P(()) ;
void pm_display_and_locate_props P(()) ;
void put_resource                P((enum res_type, char *)) ;
void read_resources              P(()) ;
void read_str                    P((char **, char *)) ;
void repaint_from_cache          P((int, int, int, int)) ;
void repaint_line                P((int)) ;
void resize_display              P((enum area_type, int, int,
                                    enum direction_type, enum direction_type)) ;
void save_cmdline                P((int, char **)) ;
void save_resources              P(()) ;
void set_dial_dims               P((int *, int *, int)) ;
void set_dims                    P((int, int)) ;
void set_frame_hints             P(()) ;
void set_prop_vals               P((char *, enum area_type,
                                    enum direction_type, int, int, int)) ;
void set_size                  P(()) ;
void set_timer                   P((int)) ;
void shift_left                  P((int, int, int, int)) ;
void start_events                P(()) ;
void start_tool                  P(()) ;
void toggle_logfile              P(()) ;
void updatedata                  P(()) ;
void usage                       P((char *)) ;
void write_cmdline               P(()) ;
void write_resources             P(()) ;
void write_sample                P(()) ;
void write_sample_header         P(()) ;
void xv_item_set_val             P((enum item_type, int)) ;
