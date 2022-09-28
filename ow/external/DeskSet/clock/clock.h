 
/*  @(#)clock.h	1.9 12/17/93
 *
 *  The DeskSet clock     (definitions).
 *
 *  Copyright (c) 1988-1992  Sun Microsystems, Inc.  
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

#include <locale.h>

#ifdef XGETTEXT
#define  MSGFILE_ERROR          "SUNW_DESKSET_CLOCK_ERR"
#define  MSGFILE_LABEL          "SUNW_DESKSET_CLOCK_LABEL"
#define  MSGFILE_MESSAGE        "SUNW_DESKSET_CLOCK_MSG"
#else
extern const char *MSGFILE_ERROR ;
extern const char *MSGFILE_LABEL ;
extern const char *MSGFILE_MESSAGE ;
#endif /*XGETTEXT*/

#define  DGET(s)                 (s)
#define  EGET(s)                 (char *) dgettext(MSGFILE_ERROR,   s)
#define  LGET(s)                 (char *) dgettext(MSGFILE_LABEL,   s)
#define  MGET(s)                 (char *) dgettext(MSGFILE_MESSAGE, s)

/* For all function declarations, if ANSI then use a prototype. */

#if  defined(__STDC__)
#define P(args)  args
#else  /* ! __STDC__ */
#define P(args)  ()
#endif  /* STDC */

#define  CLOSE                   (void) close    /* To make lint happy. */
#define  CLOSEDIR                (void) closedir
#define  EXECVP                  (void) execvp
#define  FCLOSE                  (void) fclose
#define  FPRINTF                 (void) fprintf
#define  FPUTS                   (void) fputs
#define  FREE                    (void) free
#define  GETHOSTNAME             (void) gethostname
#define  MAKE_IMAGE              (void) make_image
#define  PRINTF                  (void) printf
#define  PUTENV                  (void) putenv
#define  SPRINTF                 (void) sprintf
#define  SSCANF                  (void) sscanf
#define  STAT                    (void) stat
#define  STRCAT                  (void) strcat
#define  STRCPY                  (void) strcpy
#define  STRNCPY                 (void) strncpy

#define  DATE_ON(options)        (options->date)
#define  DIGITAL_ON(options)     (options->face == DIGITAL)
#define  IS_TIMEZONE(options)    (options->tzname    != NULL && \
                                  options->tzname[0] != '\0')
#define  SECONDS_ON(options)     (options->schoice != CS_NONE || \
                                  options->seconds == TRUE)

#define USE_SFONT(options)\
	((SECONDS_ON(options) || (options->digtype == HOURS12)) && DIGITAL_ON(options))

#define USE_DTFONT(options)\
	((options->tzval && IS_TIMEZONE(options)) || (options->date == TRUE))


/*
 * X->font is an array of 8 Xv_font's.  the first 4 are DTFONT's and
 * the second 4 are SFONT's, based on the current scale of the clock.
 * disply->fheight and display->fascent run parallel to X->font.
 */
#define INDEX_FROM_FONT_TYPE(ftype)	((((ftype) == SFONT) ? 4 : 0) + display->OLscale)


/*  Macros to improve understandability when writing away the user defined
 *  command line options.
 */

#define  ADD_CMD(s)    read_str(&argv[argc++], opts[(int) s])
#define  ADD_CMDS(str) SPRINTF(buf, "%s", str) ; read_str(&argv[argc++], buf)

#define  COLON                   ':'  /* Indices into numimage array. */
#define  SPACE                   '0' + 11

#ifndef  FALSE
#define  FALSE                   0    /* Boolean definitions. */
#endif /*FALSE*/

#ifndef  TRUE
#define  TRUE                    1
#endif /*TRUE*/

#ifndef  LINT_CAST
#ifdef   lint
#define  LINT_CAST(arg)  (arg ? 0 : 0)
#else
#define  LINT_CAST(arg)  (arg)
#endif /*lint*/
#endif /*LINT_CAST*/

#define  DIRECTORY  S_IFDIR      /* Timezone file types. */
#define  REGULAR    S_IFREG

#define  CMD_ARG(s)              !strcmp(*argv, (s))
#define  DATEBUFLEN              25
#define  DOTS(a)                 (a[0] == '.' && \
                                 (a[1] == 0 || (a[1] == '.' && a[2] == 0)))
#define  EQUAL(str, val)         !strncmp(str, val, strlen(val))
#define  FROMRIM                 15          /* Tip of hour hand to rim */
#define  INC                     argc-- ; argv++ ;
#define  LEFTEXT                 6
#define  TOPEXT                  8
#define  ITIMER_NULL             ((struct itimerval *) 0)
#define  PI                      3.1415926535
#define  SCREENHEIGHT            1152   /* "Normal" screen height in pixels. */

#define  MAXCMDOPTS              25  /* Maximum number of cmd. line options. */
#define  MAXDIGCHARS             15  /* Maximum number of "digits". */
#define  MAXFONTS                8   /* Maximum number of fonts x Max Scales. */
#define  MAXHRVAL                23  /* Max. hr value for alarm/stopwatch. */
#define  MAXIMAGES               12  /* Maximum number of digital images. */
#define  MAXMINVAL               59  /* Max. min value for alarm/stopwatch. */
#define  MAXSCALES               4   /* Maximum number of predefined scales. */
#define  MAXSECVAL               59  /* Max. sec value for alarm/stopwatch. */
#define  MAXSTRING               80  /* Maximum length of character strings. */
#define  MAXOPS                  4   /* Maximum number of rasterop types. */
#define  MAXSVTYPES              9   /* Maximum number of drawables. */
#define  MAXUSAGE                16  /* Maximum number of usage lines. */

#define  GAP                     3   /* Gap filler used in various places. */
#define  OFFSET                  5

/* Keyboard accelerators for the clock. */

#define  CHAR_12HOUR             '1'  /* Display 12 hour clock if digital. */
#define  CHAR_24HOUR             '2'  /* Display 24 hour clock if digital. */
#define  CHAR_CLOCK              'c'  /* Toggle clock face (analog/digital). */
#define  CHAR_DATE               'd'  /* Toggle date display on or off. */
#define  CHAR_ICON               'i'  /* Toggle icon (analog/roman). */
#define  CHAR_SECS               's'  /* Toggle seconds on or off. */
#define  CHAR_STOP               'S'  /* Used in stopwatch mode. */
#define  CHAR_TEST               'T'  /* Toggle test mode. */
#define  CHAR_TZONE              't'  /* Toggle between local/other zones. */
#define  CHAR_QUIT               'q'  /* Quit the clock. */

/* Property sheet row numbers for each item. */
#define  PROW_FACE               0    /* Clock face: digital, analog. */
#define  PROW_ICON               1    /* Icon display: analog, roman. */
#define  PROW_DIGITAL            2    /* Digital display: 12 hour, 24 hour. */
#define  PROW_DISPLAY            3    /* Display options: Seconds, Date. */
#define  PROW_TZONE              5    /* Timezone: local, other. */
#define  PROW_SWATCH             7    /* Stopwatch: none|reset|start|stop. */
#define  PROW_ALARM              9    /* Alarm: Hr: val   Min: val. */
#define  PROW_ACMD               10   /* Alarm command. */
#define  PROW_REPEAT             11   /* Repeat: none, once, daily. */
#define  PROW_HOURLY             13   /* Hourly command: */
#define  PROW_BUTTONS            15   /* Apply, Set Default, Reset. */

/* Various property sheet column numbers. */
#define  PCOL_ABUT               6    /* Position of the Apply button. */
#define  PCOL_ACMD               36   /* Position of "Command:" for alarm. */
#define  PCOL_DBUT               18   /* Position of Set Default button. */
#define  PCOL_HRTEXT             22   /* Position of "Hr:" text for alarm. */
#define  PCOL_JUST               18   /* Justification column for most text. */
#define  PCOL_MINTEXT            39   /* Position of "Min:" text for alarm. */
#define  PCOL_RBUT               35   /* Position of the Reset button. */
#define  PCOL_TZBUT              30   /* Position of the abbrev menu button. */
#define  PCOL_TZNAME             34   /* Position of the Timezone name. */

/*  We need to define minimum dimensions in case the user tries to resize
 *  the clock too small.
 */
 
#define  MIN_ANALOG_WIDTH        42
#define  MIN_ANALOG_HEIGHT       42
#define  MIN_DIG_WIDTH           150
#define  MIN_DIG_HEIGHT          32
#define  DEF_ANALOG_WIDTH        150
#define  DEF_ANALOG_HEIGHT       150
#define  DEF_DIG_WIDTH           145
#define  DEF_DIG_HEIGHT          70
 
/* Minimum dimensions for the scaleable digital mode digits. */
#define  MIN_FONT_HEIGHT         24     /* Height of single digit. */
#define  MIN_FONT_SPACE          4      /* Width of space between digits. */
#define  MIN_FONT_WIDTH          18     /* Width of single digit. */

extern char *getenv P((const char *)) ;
extern double rint P((double)) ;
extern time_t time P((time_t *)) ;

typedef enum { HOURS12, HOURS24 }                 DigType ;
typedef enum { DIGITAL, ANALOG, ICON_ANALOG }     Face ;
typedef enum { NORMAL,  ROMAN }                   IconType ;
typedef enum { A_NONE, A_ONCE, A_DAILY }          RepType ;
typedef enum { CS_NONE, CS_RESET, CS_START, CS_STOP } StopType ;
 
enum drawing_type { CLOCK_IMAGE, SURFACE } ;

enum font_type {
    SFONT = 0,
    DTFONT = 1
} ;


/* Spot help messages. */
enum help_type    { H_PFRAME,    H_PPANEL,   H_FCHOICE,   H_ICHOICE, H_DIGSTYLE,
                    H_DISPSTYLE, H_TZSTYLE,  H_TZBUT,     H_TZNAME,  H_SCHOICE,
                    H_ACHOICE,   H_HRCHOICE, H_MINCHOICE, H_ACMD,    H_REPEAT,
                    H_HCMD,      H_APPLY,    H_RESET,     H_DEF,     H_CANVAS,
                    H_FRAME,     H_PROPS } ;

enum hr_type      { H_AM, H_PM } ;
enum mess_type    { M_AVAL,   M_ATVAL, M_RESD, M_RESS,
                    M_TZINFO, M_FONT,  M_ICON } ;
enum op_type      { GSRC, GOR, GSET, GXOR } ;

/* Command line options. */
enum opt_type     { O_MWN,  O_PWN, O_T,     O_R,     O_12,    O_24,
                    O_ANAL, O_DIG, O_PDATE, O_MDATE, O_PSEC,
                    O_MSEC, O_V,   O_QUE,   O_HELP,  O_TZ,    O_A,
                    O_AO,   O_AD,  O_AT,    O_ACMD,  O_HCMD,  O_NAME } ;

/* Clock resources. */
enum res_type     { R_CFACE,  R_IFACE, R_DIGMODE, R_LOCAL,   R_SECHAND,
                    R_DATE,   R_TZONE, R_AHRVAL,  R_AMINVAL, R_ACHOICE,
                    R_ACMD,   R_HCMD,  R_TITLE,   R_SFONT,   R_DTFONT,
                    R_ICOLOR, R_SLEEP } ;

enum scale_type   { S_SMALL, S_MEDIUM, S_LARGE, S_EXTRALARGE } ;  /* Scales. */

enum sv_type      { HANDSPR,    SPOTPR,    DOTSPR,     ICONHANDSPR,
                    ICONANALOG, ICONROMAN, GRAY_PATCH, CLOCK_ICON,
                    CLOCK_WIN } ;

enum var_type     { V_TRUE, V_FALSE, V_TZ, V_LABEL, V_MTITLE, V_MSTR } ;

typedef enum {
    FULL_REPAINT,		/* standard repaint */
    RESIZE_REPAINT,		/* resize, scale may change */
    TIMER_REPAINT		/* timer, only hands change */
} RepaintType;

struct clckOptions {              /* Clock options. */
  Face     face ;                 /* FaceChoice value */
  Face     savedFace ;            /* FaceChoice while iconic to deal with face
                                     being overloaded. */
  IconType icontype ;             /* IconChoice value */
  char     *iconlabel ;           /* The clock icon label (if any). */
  char     *iconpath ;            /* Name of user supplied icon image file. */
  int      title ;                /* Set if frame has a titleline. */
  char     *titleline ;           /* The frame titleline, if given. */
  DigType  digtype ;              /* DigChoice value. */
  int      seconds ;              /* Seconds toggle value */
  int      date ;                 /* Date toggle value */
  int      tzval ;                /* Set to local (0) or other (1). */
  char     *localtzname ;         /* Local timezone name (from TZ). */
  char     *tzname ;              /* Timezone name. */
  char     *appname ;             /* Application name to use for resources. */
  int      ahrval ;               /* Alarm hour value. */
  int      aminval ;              /* Alarm minute value. */
  RepType  achoice ;              /* Alarm repetition choice. */
  char     *acmd ;                /* Alarm command. */
  char     *hcmd ;                /* Hourly command. */
  StopType schoice ;              /* Current stopwatch setting. */
  int      use_wincol ;           /* If set, paint icon in window colors. */
  int      userTZ ;               /* Set if user suplied a -TZ option. */
  int      doingprops ;           /* Set if props "Apply" or "Set Default". */
  int      doingrepaint ;         /* Set to prevent inc on stopwatch time. */
  int      dosleep ;              /* If set, sleep when obscured. */
  int      testing ;              /* Set if clock in test mode. */
  RepaintType	repaint_type;	  /* type of repaint (full, timer, resize) */
} ClockOptions ;

typedef struct clckOptions *Options ;

/* Coordinates & measures used to display/resize both clock faces */
 
struct displayInfo {
  char *fontnames[2] ; 		    /* X resource font names. */
  char *progname ;                  /* Name of this program. */
  enum scale_type OLscale ;         /* OPEN LOOK scale for the clock. */
  int iheight ;                     /* Initial height of the clock frame. */
  int iwidth ;                      /* Initial width of the clock frame. */
  int scale ;                       /* Scale of the digital display. */
  int x ;                           /* X start pos. of digital display. */
  int y ;                           /* Y start pos. of digital display. */
  int fascent[MAXFONTS] ;           /* Ascent of a char for each font. */
  int fheight[MAXFONTS] ;           /* Height of a char for each font. */
  int centerX ;                     /* Center of analog clock. */
  int centerY ;
  int clock_usersetsize ;           /* Set if user supplied clock size. */
  int hands_height ;                /* Height of the hands memory area. */
  int hands_width ;                 /* Width of the hands memory area. */
  int resized ;                     /* Set if the clock has been resized. */
  int started ;                     /* Set when we are ready to display. */
  struct itimerval timer ;
  struct {
    int lastSecX ;                  /* Cached last second_hand x_coord */
    int lastSecY ;                  /* Cached last second_hand y_coord */
    int lastSecX1 ;                 /* Cached last second_hand x1_coord */
    int lastSecY1 ;                 /* Cached last second_hand y1_coord */
  } secondhand ;
  struct {                          /* Cached last hands coord */
    int angle1 ;
    int angle2 ;
    int width ;
    int radius ;                    /* Circle radius */
  } hands ;
} DisplayInfo ;

typedef struct displayInfo *ClockDisplay ;

struct clckObject {            /* The clock object. */
  int cheight ;                /* Height of area, clock can be drawn in. */
  int height ;                 /* Current height of the open window. */
  int width ;                  /* Current width of the open window. */
  struct tm *tm ;              /* Current time value. */
  int hours ;                  /* Currently display hour value. */
  int mins ;                   /* Currently displayed minute value. */
  int secs ;                   /* Current second value. */
  int latest_digital_w ;       /* Cached width value for init_digital. */
  int latest_digital_h ;       /* Cached height value for init_digital. */
} ClockObject ;

typedef struct clckObject *Clock ;

char *get_resource        P((enum res_type)) ;
char *prop_get_str        P(()) ;
char *set_bool            P((int)) ;

int armwidth              P((int)) ;
int check_alarm_setting   P((char *)) ;
int get_bool_resource     P((enum res_type, int *)) ;
int get_cmdstr            P((char **, char **)) ;
int get_int_resource      P((enum res_type, int *)) ;
int get_str_resource      P((enum res_type, char *)) ;
int get_str_width         P((enum font_type, char *)) ;
int main                  P((int, char **)) ;
int rotx                  P(()) ;
int roty                  P(()) ;
int run_command           P((char *)) ;

void adjust_date_size     P(()) ;
void adjust_timezone_size P(()) ;
void analog_repaint       P((Face)) ;
void beep                 P(()) ;
void build_numbers        P((int, int)) ;
void center               P((int, int, int *, int *, int, int)) ;
void check_args           P(()) ;
void clock_exec           P((char **)) ;
void dig_repaint          P(()) ;
void display_prop_sheet   P(()) ;
void do_char              P((int)) ;
void do_clock             P((int, char **)) ;
void do_drag              P(()) ;
void do_stopwatch         P((StopType)) ;
void draw_circle          P((enum sv_type, int)) ;
void erase_second_hand    P(()) ;
void get_font             P((int, int, enum font_type)) ;
void get_font_scale       P((char *)) ;
void get_frame_size       P((int *, int *, int *, int *)) ;
void get_options          P((int, char **)) ;
void get_time             P(()) ;
void grow_font            P((int)) ;
void handle_repaint       P((Face)) ;
void handle_resize        P(()) ;
void handle_time          P((Face)) ;
void handle_timer         P(()) ;
void init_analog          P((int, int)) ;
void init_cmdline_opts    P(()) ;
void init_digital         P((int, int)) ;
void init_display         P(()) ;
void init_gray_patch      P(()) ;
void init_numbers         P(()) ;
void init_icon            P(()) ;
void init_icon_images     P(()) ;
void init_options         P(()) ;
void init_opt_vals        P(()) ;
void init_points          P(()) ;
void init_text            P(()) ;
void load_deskset_defs    P(()) ;
void load_resources       P(()) ;
void make_canvas          P(()) ;
void make_frame           P(()) ;
void make_graphics        P((int *, char **)) ;
void make_gcs             P(()) ;
void paint_clock          P((Face)) ;
void paint_date           P((Face)) ;
void paint_dig_seconds    P(()) ;
void paint_hand           P((enum drawing_type, enum sv_type, int, int, int,
                            int, int, int, int, int)) ;
void paint_hands          P((enum drawing_type, enum sv_type, int, int, int)) ;
void paint_second_hand    P(()) ;
void paint_stopwatch      P((Face)) ;
void paint_ticks          P((enum sv_type, enum sv_type, int)) ;
void paint_timezone       P((int)) ;
void put_resource         P((enum res_type, char *)) ;
void read_resources       P((char *)) ;
void read_str             P((char **, char *)) ;
void save_cmdline         P((int, char **)) ;
void save_resources       P(()) ;
void set_clock_props      P(()) ;
void set_def_dims         P(()) ;
void set_frame_hints      P(()) ;
void set_frame_size       P((int, int, int, int)) ;
void set_min_frame_size   P(()) ;
void set_option_values    P(()) ;
void set_stopwatch_val    P((StopType)) ;
void set_timer            P((int, int, int, int)) ;
void set_timezone         P((char *)) ;
void set_tzval            P((int)) ;
void start_tool           P(()) ;
void usage                P((char *)) ;
void write_cmdline        P(()) ;
void write_resources      P(()) ;
void xclear               P((enum drawing_type, enum sv_type,
                            int, int, int, int)) ;
void ximage               P((enum drawing_type, enum sv_type, enum op_type,
                            int, int, int, int, enum sv_type)) ;
void xline                P((enum drawing_type, enum sv_type, enum op_type,
                            int, int, int, int)) ;
void xnumimage            P((int, int, int, int, int)) ;
void xtext                P((enum font_type, int, int, char *)) ;
