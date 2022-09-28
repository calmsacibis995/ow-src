
/*  @(#)snapshot.h	3.7 11/20/96
 *
 *  Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
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
#include <sys/param.h>
#include <memory.h>

#ifdef SVR4
#include <pixrect/rasterfile.h>
#else
#include <rasterfile.h>
#endif "SVR4"

#include <sys/time.h>
#include <locale.h>

#ifdef XGETTEXT
#define  MSGFILE		"SUNW_DESKSET_SNAPSHOT_MSG"
#define  LBLFILE         "SUNW_DESKSET_SNAPSHOT_LABEL"
#else
extern char *MSGFILE;
extern char *LBLFILE;
#endif

#define  EGET(s)         (char *) dgettext("SUNW_DESKSET_SNAPSHOT_ERR",   s)
#define  LGET(s)         (char *) dgettext(LBLFILE, s)
#define  MGET(s)         (char *) dgettext(MSGFILE, s)
#define  DGET(s)         (s)

/* For all function declarations, if ANSI then use a prototype. */

#if  defined(__STDC__)
#define P(args)  args
#else  /* ! __STDC__ */
#define P(args)  ()
#endif  /* STDC */

#define  FCLOSE          (void) fclose
#define  FPRINTF         (void) fprintf
#define  FREE            (void) free
#define  GETCWD          (void) getcwd
#define  MEMCPY          (void) memcpy
#define  PCLOSE          (void) pclose
#define  PRINTF          (void) printf
#define  SELECT          (void) select
#define  SPRINTF         (void) sprintf
#define  STRCAT          (void) strcat
#define  STRCPY          (void) strcpy
#define  STRNCPY         (void) strncpy
#define  UNLINK          (void) unlink

#define  DUMP_WINDOW     0        /* Snapshot dump types. */
#define  DUMP_RECT       1
#define  DUMP_SCREEN     2

#define  CHANGE_DIR      (void) change_dir
#define  CLIPBOARD_NAME  (void) clipboard_name

#define  ISNONE          0        /* Possible snapshot load/snap status. */
#define  ISSNAP          1
#define  ISLOAD          2
#define  ISDRAG		 3
#define	 ISQUIT		 4

				  /* Raster print options. */
#ifdef SVR4
#define  PRINTER_OPT     'd'
#else /* SVR4 */
#define  PRINTER_OPT     'P'
#endif /* SVR4 */
#define  ROTATE_OPT      'r'
#define  DOUBLE_OPT      '2'
#define  WIDTH_OPT       'W'
#define  HEIGHT_OPT      'H'
#define  SCALE_OPT       's'
#define  POSITION_OPT    'l'
#define  MONO_OPT        'm'

#define  MARGIN_OFFSET	 1	  /* Margin offset for all panels */

#define  MR_LSSPD        0        /* Main panel row offsets. */
#define  MR_STYPE        1
#define  MR_DELAY        2
#define  MR_BELL         3
#define  MR_HIDE         4
#define  MR_SNAP_VIEW    5

#define  MC_LOAD         0        /* Main panel column offsets. */

#define  LSR_DIR         0        /* Load/Save panel row/column offsets. */
#define  LSR_FILE        1
#define  LSR_BUTTON      2

#define  PR_DEST         0         /* Print panel row offsets. */
#define  PR_PNAME_DIR    1
#define  PR_PFILE        2
#define  PR_PORIENT      3
#define  PR_POS          4
#define  PR_SPEC         5
#define  PR_SCALE        6
#define  PR_W_H          7
#define  PR_DBITS        8
#define  PR_PRINT        9

#define  P_COL(ptype, col)  xv_col(X->panel[(int) ptype], col)
#define  P_ROW(ptype, row)  xv_row(X->panel[(int) ptype], row)

/* Command line options (for saving). */
enum cmd_type { CMD_DEFDIR, CMD_DEFFILE, CMD_GRAY, CMD_LOAD, CMD_NOCONFIRM } ;

enum cur_type   { C_CONFIRM, C_NULL, C_SELECT } ;      /* Cursor types. */

enum dump_type  { D_RECT, D_SCREEN, D_WINDOW } ;       /* Dump types. */

/* Frame types. */
#ifdef FILECHOOSER
enum frame_type { F_MAIN, F_PRINT, F_ROOT, F_LOAD, F_SAVE, F_VIEW, F_ALL } ;
#else
enum frame_type { F_MAIN, F_PRINT, F_ROOT, F_LOADSAVE, F_VIEW, F_ALL } ;
#endif

enum panel_type { P_MAIN, P_PRINT, P_LOADSAVE } ;      /* Panel types. */

enum rasio_type { R_DATA, R_FILE } ;    /* Rasterfile load/save i/o types. */

enum rect_type  { R_MAIN, R_ROOT, R_BOX } ;   /* Rect types. */

/* Server image types. */
enum sv_type    { SV_DRAG_PTR, SV_DROP_PTR } ;

/* Panel items. */

enum pi_type { PI_BELL,      PI_DELAY,   PI_LSDIR,    PI_LSFILE,
               PI_LSBUT,     PI_HIDEWIN, PI_SNAP,     PI_VIEW,
	       PI_DRAG,      PI_STYPE,   PI_DEST,     PI_DBITS,
	       PI_HEIGHTT,   PI_HEIGHTV, PI_PDIR,     PI_PFILE,
	       PI_POSITION,  PI_PORIENT, PI_PNAME,    PI_SCALE,
	       PI_WIDTHT,    PI_WIDTHV,  PI_LEFTPOSV, PI_BOTPOSV, 
	       PI_LEFTPOSM,  PI_BOTPOSM, PI_MONO,     PI_PRBUT } ;

enum mes_type { M_GETRECT,  M_GETWIN,   M_ADELAY,   M_ABORT,    M_OUTMEM,
                M_BACKIN8,  M_BEBACK,   M_COPYING,  M_NOWIN,    M_PRINTING,
                M_CPRINT,   M_FPRINT,   M_NODIR,    M_BADPATH,  M_NOOPEN,
                M_TAKING,   M_SAVEFAIL, M_SAVESUCS, M_SNAPSUCS, M_CANCEL,
                M_SCANCEL,  M_REDUCE,   M_LOADING,  M_NOTRAST,  M_BADCOL,
                M_NOREAD,   M_LOADSING, M_LOADSUCS, M_COLOR,    M_NOSIMAGE,
                M_NOVIMAGE, M_NOPIMAGE, M_COMPRESS, M_ADDCOLS,  M_ADJUST,
		M_DITHER24, M_DITHER8,  M_EXPAND24, M_EXPAND8,  M_GRAY,
                M_RASTCHK,  M_GIFCHK,	M_UNRECOG,  M_NOSNAP,   M_DELAY,
		M_ISDIR,    M_ISNOTREG, M_NOACCESS, M_NOTOPEN,  M_TFAIL,
		M_LAUNCH,   M_NOLAUNCH } ;

enum ostr_type { O_CONTINUE,   O_CANCEL,  O_POPT,     O_PMITEM,
                 O_OMITEM,     O_VIEW,    O_CMSNAME,  O_FOVERWRITE,
                 O_LOVERWRITE, O_SOVERWRITE, O_YES,   O_NO,
		 O_NOSRC,      O_ALERT1,  O_ALERT2,   O_ALERT3,
		 O_ALERT4,     O_ALERT5,  O_FONT,     O_ONEFILE,
		 O_OERR,       O_NORAST,  O_RNDNAME,  O_RAST8T1,
		 O_REDUCE,     O_DEPTH,   O_CMAPERR,  O_LERR,
		 O_NOWIN,      O_FLABEL,  O_NOAPP,    O_CONOVER,
		 O_TOOL,       O_PDEST,   O_PRINT,    O_NOCDIR,
		 O_SETNAME,    O_BADIDEP, O_BADSDEP,  O_DEFF,
		 O_LOAD,       O_SAVE,    O_LDISCARD, O_LCANCEL,
		 O_SDISCARD,   O_SCANCEL, O_DCANCEL,  O_QLDISCARD,
		 O_QSDISCARD,  O_QCANCEL, O_VERSION,  O_USAGE1,
		 O_USAGE2,     O_SETNAMEFILE } ;

enum pstr_type { DELAY0,    DELAY2,      DELAY4,      DELAY8,     DELAY16,
                 TIMER,     SAVE_SNAP,   LOAD_SNAP,   PRINT_SNAP,
                 ST_SCREEN, ST_REGION,   ST_WINDOW,   DO_SNAP,    VIEW_SNAP,
                 P_LABEL,   L_LABEL,     S_LABEL,     DEST1,
                 DEST2,     DEF_PRINTER, ORIENTU,     ORIENTS,    CENTER,
		 SPECIFY,   DEF_LEFTP,
                 PLPOS,     DEF_BOTP,    PBPOS,       SCALE1,     SCALE2,
                 SCALE3,    SCALE4,      DEF_WIDTH,   DEF_HEIGHT, DBITS1,
                 DBITS2,    MONO,        PBUTTON } ;

enum help_type { H_MENUOPT, H_LSDIR,    H_LSFILE,  H_DELAY,   H_STYPE,
                 H_BELL,    H_HIDEWIN,  H_SCREEN,  H_REGION,  H_WINDOW,
                 H_SOURCE,  H_SAVE,     H_LOAD,    H_PRINT,   H_VIEW,
                 H_DEST,    H_PNAME,    H_PFILE,   H_ORIENT,  H_POS,
                 H_APPEAR,  H_DOUBLE,   H_MONO,    H_SNAP,    H_PBUTTON, 
		 H_LSBUTTON } ;

#define  MAXCURS    3      /* Maximum number of cursor types. */
#define  MAXFRAMES  6      /* Maximum number of frames. */
#define  MAXPANELS  3      /* Maximum number of panels. */
#define  MAXPITEMS  31     /* Maximum number of panel items. */
#define  MAXRECTS   3      /* Maximum number of rects. */
#define  MAXSVS     3      /* Maximum number of server images. */

typedef enum {
	RASTERFILE,
	GIFFILE,
} sn_filetype;

#ifndef  LINT_CAST
#ifdef   lint
#define  LINT_CAST(arg)    (arg ? 0 : 0)
#else
#define  LINT_CAST(arg)    (arg)
#endif /*lint*/
#endif /*LINT_CAST*/

#ifndef  FALSE             /* Boolean definitions. */
#define  FALSE      0
#endif /*FALSE*/

#ifndef  TRUE
#define  TRUE       1
#endif /*TRUE*/

#define  IMAGELEN(w, h, d)  ((((((w) * (d)) + (RASTPAD-1)) >> 3) &~ 1) * (h))
#define  LINELEN(w, d)       (((((w) * (d)) + (RASTPAD-1)) >> 3) &~ 1)

#define  MID            0    /* Directions. */
#define  N              1
#define  NE             2
#define  E              3
#define  SE             4
#define  S              5
#define  SW             6
#define  W              7
#define  NW             8

#define  CMAP_SIZE      256   /* Colormap size. */
#define  DEFAULT_DELAY  0
#define  M_MTEXTLEN     23
#define  M_LSTEXTLEN    53
#define	 RASTPAD(d)	(d < 17 ? 16 : d) /* Rasterfile rows padded to 16 */
					  /* bits if image is 1 or 8 bit, */
					  /* otherwise, no padding.       */

typedef struct {              /* Description of rasterfile image. */
  int type ;                  /* Type of image. */
  int width ;                 /* Width in pixels. */
  int height ;                /* Height in pixels. */
  int depth ;                 /* Depth of image. */
  int cmaptype ;              /* Colormap type. */
  int cmapused ;              /* Number of entries used in colormap. */
  int bytes_per_line ;        /* Accelerator to the next image line. */
  int used_malloc ;           /* If set, image data created via XGetImage. */
  unsigned char *red ;        /* Red colormap entries. */
  unsigned char *green ;      /* Green colormap entries. */
  unsigned char *blue ;       /* Blue colormap entries. */
  unsigned char *data ;       /* Image data. */
} image_t ;

struct snapVars {            /* Snapshot variables and options. */
  struct itimerval tv ;

  char *ttid;	     	     /* tt message handler id */
  int no_imagetool;          /* True if couldn't start imagetool */
  int autoload ;             /* If set, file loaded and viewed automaticaly. */
  int charheight ;           /* Height of snapshot font. */
  int charwidth ;            /* Width of snapshot font. */
  int debug_on ;             /* If set, prints out debugging information. */
  int delay_count ;          /* Counter used in callbacks. */
  int depth ;                /* Screen depth. */
  int num_vis ;		     /* Number of visuals. */
  int havename ;             /* There is a name associated with this snap. */
  sn_filetype filetype;	     /* Type of file/image we are dealing with */
  int hide ;                 /* If set, hide snapshot window first. */
  int isgray ;               /* Set if this is a grayscale display. */
  int ispopen ;              /* Set if we have a zcat pipe open. */
  int ls_status ;            /* Current snapshot load/snap status. */
  int noconfirm ;            /* Prompt user before overwriting? */
  int snapshot_delay ;       /* Seconds to delay */
  int timer_bell_on ;        /* Keep state for graying out. */
  int grasps[3][3] ;         /* Places where a window can be "grabbed". */

  double gamma_val; 		 /* Gamma correction value */
  int gamma_tbl[256];		 /* Gamma correction table */

  char cdir[MAXPATHLEN] ;    /* Current directory. */
  char path[MAXPATHLEN] ;    /* Full pathname. */
  char *tool_name ;
  char *default_dir ;        /* Default directory name for load/save popup. */
  char *default_file ;       /* Default filename for load/save popup. */
  char *file_pos ;

  char directory_name[MAXPATHLEN] ;
  char file_name[MAXPATHLEN] ;

  FILE *fp ;                 /* File pointer for disk rasterfile i/o. */
  unsigned char *rbuf ;      /* Pointer to internal rasterfile buffer. */

  image_t *image ;           /* Description of current image. */
} SnapVars ;

typedef struct snapVars *Vars ;

extern char *getenv P((const char *)) ;
extern char *getwd P((char *)) ;
extern char *mktemp P((char *)) ;
extern char *strrchr P((const char *, int)) ;
extern char *strtok P((char *, const char *)) ;
extern FILE *fopen P((const char *, const char *)) ;

char *base_name P((char *)) ;
char *clipboard_name P((char *)) ;
char *get_char_val P((enum pi_type)) ;

FILE *openfile P((char *)) ;

image_t *CreateImageFromImg P(()) ;
image_t *compress P((image_t *)) ;
image_t *copy_image P((image_t *)) ;
image_t *dither24to8 P((image_t *)) ;
image_t *dither8to1 P((image_t *)) ;
image_t *expand8to24 P((image_t *)) ;
image_t *expand1to8 P((image_t *)) ;
image_t *gif_load P((char *, enum rasio_type)) ;
image_t *gray P((image_t *)) ;
image_t *new_image P(()) ;
image_t *rast_load P((char *, enum rasio_type)) ;

int change_dir P(()) ;
int check_directory P((char *, char *)) ;
int check_overwrite P((int)) ;
int check_pathname P((char *)) ;
int confirm P((char *)) ;
int do_overwrite P((enum ostr_type, int)) ;
int dragdrop_init P(()) ;
int dump_rasterfile P((image_t *, enum rasio_type)) ;
int get_area P((enum dump_type, int *, int *)) ;
int get_drag_name P(()) ;
int get_image_cmap P((enum rasio_type,
                      register struct rasterfile *, image_t *)) ;
int get_image_header P((enum rasio_type, register struct rasterfile *)) ;
int get_int_val P((enum pi_type)) ;
int get_raster_len P((image_t *)) ;
int imagelen P((int, int, int)) ;
int linelen P((int, int)) ;
int load_file P(()) ;
int main P((int, char **)) ;
int pad_value P((int)) ;
int place_drop_site P(()) ;
int set_image_colormap P((image_t *)) ;
int sn_fgetc P((enum rasio_type)) ;
int sn_fread P((char *, int, int, enum rasio_type)) ;
int verify_paths P((int, enum pi_type, enum pi_type, char [])) ;
int warn P((char *)) ;
int loadfile P((char *, enum rasio_type)) ;

unsigned char *copy_imagedata P((image_t *)) ;
unsigned char *get_image P((enum rasio_type, register struct rasterfile *)) ;

void activate_item P((enum pi_type, int)) ;
void add_colors P((image_t *)) ;
void adjust_image P((struct rasterfile *, unsigned char *, int));
void *ck_mmap P((char *, size_t *)) ;
void *ck_unmap P((caddr_t)) ;
void *ck_zfree P((caddr_t, int)) ;
void *ck_zmalloc P((size_t)) ;
void closefile P((FILE *)) ;
void copyrect P(()) ;
void destroy_frame P((enum frame_type)) ;
void destroy_pr P(()) ;
void disable_timer P(()) ;
void do_delay P(()) ;
void do_destination P((int)) ;
void do_rect_dump P(()) ;
void do_position P((int)) ;
void do_scaleto P((int)) ;
void do_screen_dump P(()) ;
void do_snapshot P((int, char **)) ;
void do_snap_type P(()) ;
void do_sync P((enum frame_type)) ;
void do_view P(()) ;
void do_window_dump P(()) ;
void frame_done P(()) ;
void frame_show P((enum frame_type, int)) ;
void free_fullscreen P(()) ;
void free_image P((image_t *)) ;
void init_graphics P((int *, char **)) ;
void init_multivis P(()) ;
void init_pad P(()) ;
void init_text P(()) ;
void init_vars P(()) ;
void make_cursors P(()) ;
void make_frame P(()) ;
void make_fullscreen P((enum dump_type)) ;
void make_icon P(()) ;
void make_images P(()) ;
void make_main_panel P(()) ;
void message P((char *)) ;
void print_snapfile P(()) ;
void reverse_x P((int *)) ;
void reverse_y P((int *)) ;
void save_cmdline P((int, char **)) ;
void savefile P((char *, enum rasio_type, int)) ;
void set_current_pad P((int, int)) ;
void set_label P((enum frame_type, char *)) ;
void set_namestripe P(()) ;
void set_time_delay P((struct itimerval *)) ;
void show_item P((enum pi_type, int)) ;
void sound_bell P((struct timeval)) ;
void start_timer P(()) ;
void start_tool P(()) ;
void viewfile P(()) ;
void old_viewfile P(()) ;
void write_cmdline P(()) ;
void stop_dragdrop P(()) ;
