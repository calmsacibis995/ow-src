
/*  @(#)calctool.h 1.12 94/04/28 Copyright (c) Sun Microsystems Inc.
 *
 *  Copyright (c) 1987 - 1990, Sun Microsystems, Inc.  All Rights Reserved.
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

#include <locale.h>

#ifdef XGETTEXT
#define  MSGFILE_ERROR    "SUNW_DESKSET_CALCTOOL_ERR"
#define  MSGFILE_LABEL    "SUNW_DESKSET_CALCTOOL_LABEL"
#define  MSGFILE_MESSAGE  "SUNW_DESKSET_CALCTOOL_MSG"
#else
extern char *MSGFILE_ERROR ;
extern char *MSGFILE_LABEL ;
extern char *MSGFILE_MESSAGE ;
#endif

#define  DGET(s)          (s)
#define  EGET(s)          (char *) dgettext(MSGFILE_ERROR,   s)
#define  LGET(s)          (char *) dgettext(MSGFILE_LABEL,   s)
#define  MGET(s)          (char *) dgettext(MSGFILE_MESSAGE, s)

/* For all function declarations, if ANSI then use a prototype. */

#if  defined(__STDC__)
#define P(args)  args
#else  /* ! __STDC__ */
#define P(args)  ()
#endif  /* STDC */

extern char *getenv   P((const char *)) ;
extern char *getwd    P((char *)) ;
extern char *mktemp   P((char *)) ;

extern double drand48 P(()) ;

#define  MP_SIZE      150     /* Size of the multiple precision values. */

#define  CLOSE        (void) close      /* To make lint happy. */
#define  FCLOSE       (void) fclose
#define  FFLUSH       (void) fflush
#define  FGETS        (void) fgets
#define  FPRINTF      (void) fprintf
#define  FPUTS        (void) fputs
#define  GETHOSTNAME  (void) gethostname
#define  IOCTL        (void) ioctl
#define  MEMCPY       (void) memcpy
#define  MEMSET       (void) memset
#define  MKTEMP       (void) mktemp
#define  PUTC         (void) putc
#define  READ         (void) read
#define  REWIND       (void) rewind
#define  SELECT       (void) select
#define  SIGNAL       (void) signal
#define  SPRINTF      (void) sprintf
#define  SSCANF       (void) sscanf
#define  STRCAT       (void) strcat
#define  STRCPY       (void) strcpy
#define  STRNCAT      (void) strncat
#define  STRNCPY      (void) strncpy
#define  UNLINK       (void) unlink
#define  WRITE        (void) write

/* Various pseudo events used by the calctool program. */
#define  KEYBOARD_DOWN    100    /* Keyboard character was pressed. */
#define  KEYBOARD_UP      101    /* Keyboard character was released. */
#define  LEFT_DOWN        102    /* Left mouse button was depressed. */
#define  LEFT_UP          103    /* Left mouse button was debounced. */
#define  MIDDLE_DOWN      104    /* Middle mouse button was depressed. */
#define  MIDDLE_UP        105    /* Middle mouse button was debounced. */
#define  RIGHT_DOWN       106    /* Right mouse button was depressed. */
#define  RIGHT_UP         107    /* Right mouse button was debounced. */
#define  MOUSE_DRAGGING   108    /* The mouse is being dragged. */
#define  TAKE_FROM_SHELF  109    /* PUT function key was pressed. */
#define  PUT_ON_SHELF     110    /* GET function key was pressed. */
#define  SHOWHELP         111    /* F1 key was pressed; show help message. */
#define  LASTEVENTPLUSONE 112    /* Not one of the above. */

enum base_type { BIN, OCT, DEC, HEX } ;      /* Base definitions. */

/* Button states. */
enum but_state { B_NORMAL, B_INVERT, B_STENCIL, B_NULL } ;
 
/* Command line options (for saving). */
enum cmd_type { CMD_2D,      CMD_3D,   CMD_ACC,    CMD_COLOR,
                CMD_LEFTH,   CMD_MONO, CMD_RIGHTH, CMD_TITLE,
                CMD_NOTITLE, CMD_NAME, CMD_WN } ;

/* Button corners. */
enum corner_type { TL_CORNER, TR_CORNER, BL_CORNER, BR_CORNER } ;

/* Frame/Canvas/Pixwin types. */
enum fcp_type  { FCP_KEY, FCP_REG, FCP_MODE } ;

enum font_type { BFONT, MFONT, NFONT, SFONT } ;   /* Text font definitions. */

/* Help string types. */
enum help_type { H_ACC,    H_BASE,    H_CON,     H_EXCH,
                 H_FUN,    H_LSHF,    H_MODE,    H_NUM,
                 H_RCL,    H_RSHF,    H_STO,     H_TRIG,       H_PROPS,
                 H_ABUT,   H_AFRAME,  H_APANEL,  H_APPEARANCE,
                 H_APPLY,  H_ATEXT,   H_CFCBUT,  H_CFDESC,
                 H_CFFBUT, H_CFFRAME, H_CFNO,    H_CFPANEL,
                 H_CFVAL,  H_DEF,     H_DISPLAY, H_MCANVAS,
		 H_MFRAME, H_PFRAME,
                 H_PPANEL, H_RESET,   H_STYLE } ;

/* Pseudo panel items. */
enum item_type { BASEITEM, DISPLAYITEM, TTYPEITEM, NUMITEM,
                 HYPITEM,  INVITEM,     OPITEM,    MODEITEM } ;

/* XView labels for various items. */
enum label_type { L_LCALC, L_UCALC, L_CONNO, L_NEWCON, L_FUNNO, L_NEWFUN } ;

/* Popup menu types. */
enum menu_type { M_ACC,  M_BASE, M_CON,  M_EXCH, M_FUN,  M_LSHF,  M_MODE,
                 M_NUM,  M_RCL,  M_RSHF, M_STO,  M_TRIG, M_PROPS, M_NONE
} ;

/* Message string types. */
enum mess_type { MESS_COLOR, MESS_FONT, MESS_MONO, MESS_PARAM,
                 MESS_ICON,  MESS_CON } ;

/* Calculator modes. */
enum mode_type { BASIC, FINANCIAL, LOGICAL, SCIENTIFIC } ;

enum mp_type { MP_ADD2A, MP_ADD2B, MP_PART1, MP_ASIN,   /* MP error types. */
               MP_ATAN,  MP_CHKC,
               MP_CHKD,  MP_CHKE,  MP_CHKF,  MP_CHKG,
               MP_CHKH,  MP_CHKI,  MP_CHKJ,  MP_CHKL,
               MP_CHKM,  MP_CHKN,  MP_CMD,   MP_CMR,
               MP_CQM,   MP_DIVA,  MP_DIVB,  MP_DIVIA,
               MP_DIVIB, MP_EXPA,  MP_EXPB,  MP_EXP1,
               MP_LNA,   MP_LNB,   MP_LNSA,
               MP_LNSB,  MP_LNSC,  MP_MULA,  MP_MULB,
               MP_MULC,  MP_MUL2A, MP_MUL2B, MP_MULQ,
               MP_NZRA,  MP_NZRB,  MP_NZRC,
               MP_OVFL,  MP_PI,    MP_PWRA,
               MP_PWRB,  MP_PWR2A, MP_PWR2B, MP_RECA,
               MP_RECB,  MP_RECC,  MP_RECD,  MP_ROOTA,
               MP_ROOTB, MP_ROOTC, MP_ROOTD,
               MP_ROOTE, MP_ROOTF, MP_SETB,
               MP_SETC,  MP_SETD,  MP_SETE,  MP_SIN,
               MP_SIN1,  MP_SQRT,  MP_TAN } ;

enum num_type { ENG, FIX, SCI } ;            /* Number display mode. */

enum op_type { OP_SET, OP_CLEAR, OP_NOP } ;  /* Operation item settings. */

enum opt_type { O_ACCVAL, O_ACCRANGE, O_BASE, O_DISPLAY, O_MODE, O_TRIG, } ;

/* Resources. */
enum res_type { R_ACCURACY, R_BASE,     R_DISPLAY,  R_MODE,     R_MONO,
                R_REGS,     R_RHAND,    R_THREED,   R_TITLE,    R_TRIG,
                R_DECDIG,   R_HEXDIG,   R_ARITHOP,  R_ADJUST,   R_PORTION,
                R_FUNC,     R_MAINMODE, R_PLOGICAL, R_BLOGICAL, R_FIN,
                R_TRIGMODE, R_TRIGCOL,  R_SCI,      R_BACK,     R_DISPCOL,
                R_MEMORY,   R_TEXT,     R_BUTFONT,  R_MODEFONT, R_MEMFONT,
                R_DISPFONT, R_BEEP
} ;

enum scale_type { S_SMALL, S_MEDIUM, S_LARGE, S_EXTRALARGE } ;  /* Scales. */

enum trig_type { DEG, GRAD, RAD } ;          /* Trigonometric types. */

enum usage_type { USAGE1, USAGE2, USAGE3 } ;    /* Usage message types. */

enum var_type { V_ASC,      V_BSP,      V_CALCTOOL, V_CANCEL,
                V_CANVAS,   V_CLR,      V_CONFIRM,  V_CONTINUE,
                V_CONWNAME, V_DEL,      V_DISP,     V_ERROR,
                V_FALSE,    V_FUNWNAME, V_HYP,      V_INV,
                V_INVCON,   V_INVALID,  V_LCON,     V_LFUN,
                V_NOCHANGE, V_NUMSTACK, V_OPSTACK,  V_OWRITE,
                V_RANGE,    V_TRUE,     V_UCON,     V_UFUN } ;

/* Abbreviations for the calctool keyboard and menu equivalents. */

#define  KEY_D     buttons[v->righthand ?  4 :  0].value       /* d */
#define  KEY_E     buttons[v->righthand ?  5 :  1].value       /* e */
#define  KEY_F     buttons[v->righthand ?  6 :  2].value       /* f */
#define  KEY_CLR   buttons[v->righthand ?  7 :  3].value       /* del */
#define  KEY_INT   buttons[v->righthand ?  2 :  4].value       /* CTL('i') */
#define  KEY_FRAC  buttons[v->righthand ?  3 :  5].value       /* CTL('f') */
#define  KEY_BASE  buttons[v->righthand ?  1 :  6].value       /* B */
#define  KEY_DISP  buttons[v->righthand ?  0 :  7].value       /* D */

#define  KEY_A     buttons[v->righthand ? 12 :  8].value       /* a */
#define  KEY_B     buttons[v->righthand ? 13 :  9].value       /* b */
#define  KEY_C     buttons[v->righthand ? 14 : 10].value       /* c */
#define  KEY_BSP   buttons[v->righthand ? 15 : 11].value       /* CTL('h') */
#define  KEY_ABS   buttons[v->righthand ? 10 : 12].value       /* CTL('u') */
#define  KEY_CHS   buttons[v->righthand ? 11 : 13].value       /* C */
#define  KEY_KEYS  buttons[v->righthand ?  9 : 14].value       /* k */
#define  KEY_MODE  buttons[v->righthand ?  8 : 15].value       /* M */

#define  KEY_7     buttons[v->righthand ? 20 : 16].value       /* 7 */
#define  KEY_8     buttons[v->righthand ? 21 : 17].value       /* 8 */
#define  KEY_9     buttons[v->righthand ? 22 : 18].value       /* 9 */
#define  KEY_MUL   buttons[v->righthand ? 23 : 19].value       /* x */
#define  KEY_REC   buttons[v->righthand ? 18 : 20].value       /* r */
#define  KEY_SQR   buttons[v->righthand ? 19 : 21].value       /* @ */
#define  KEY_ACC   buttons[v->righthand ? 17 : 22].value       /* A */
#define  KEY_MEM   buttons[v->righthand ? 16 : 23].value       /* m */

#define  KEY_4     buttons[v->righthand ? 28 : 24].value       /* 4 */
#define  KEY_5     buttons[v->righthand ? 29 : 25].value       /* 5 */
#define  KEY_6     buttons[v->righthand ? 30 : 26].value       /* 6 */
#define  KEY_DIV   buttons[v->righthand ? 31 : 27].value       /* / */
#define  KEY_PER   buttons[v->righthand ? 26 : 28].value       /* % */
#define  KEY_SQRT  buttons[v->righthand ? 27 : 29].value       /* s */
#define  KEY_CON   buttons[v->righthand ? 25 : 30].value       /* # */
#define  KEY_FUN   buttons[v->righthand ? 24 : 31].value       /* F */

#define  KEY_1     buttons[v->righthand ? 36 : 32].value       /* 1 */
#define  KEY_2     buttons[v->righthand ? 37 : 33].value       /* 2 */
#define  KEY_3     buttons[v->righthand ? 38 : 34].value       /* 3 */
#define  KEY_SUB   buttons[v->righthand ? 39 : 35].value       /* - */
#define  KEY_LPAR  buttons[v->righthand ? 34 : 36].value       /* ( */
#define  KEY_RPAR  buttons[v->righthand ? 35 : 37].value       /* ) */
#define  KEY_RCL   buttons[v->righthand ? 33 : 38].value       /* R */
#define  KEY_STO   buttons[v->righthand ? 32 : 39].value       /* S */

#define  KEY_0     buttons[v->righthand ? 44 : 40].value       /* 0 */
#define  KEY_PNT   buttons[v->righthand ? 45 : 41].value       /* . */
#define  KEY_EQ    buttons[v->righthand ? 46 : 42].value       /* = */
#define  KEY_ADD   buttons[v->righthand ? 47 : 43].value       /* + */
#define  KEY_EXP   buttons[v->righthand ? 42 : 44].value       /* E */
#define  KEY_ASC   buttons[v->righthand ? 43 : 45].value       /* CTL('a') */
#define  KEY_EXCH  buttons[v->righthand ? 41 : 46].value       /* X */
#define  KEY_QUIT  buttons[v->righthand ? 40 : 47].value       /* q */

#define  ACC_START 0                             /* 0 */
#define  ACC_END   9                             /* 9 */

#define  MEM_START 10                            /* 0 */
#define  MEM_END   19                            /* 9 */

#define  SFT_START 20                            /* 0 */
#define  SFT_END   34                            /* f */

#define  BASE_BIN  menu_entries[35]              /* b */
#define  BASE_OCT  menu_entries[36]              /* o */
#define  BASE_DEC  menu_entries[37]              /* d */
#define  BASE_HEX  menu_entries[38]              /* h */

#define  DISP_ENG  menu_entries[39]              /* e */
#define  DISP_FIX  menu_entries[40]              /* f */
#define  DISP_SCI  menu_entries[41]              /* s */

#define  TRIG_DEG  menu_entries[42]              /* d */
#define  TRIG_GRA  menu_entries[43]              /* g */
#define  TRIG_RAD  menu_entries[44]              /* r */

#define  MODE_BAS  menu_entries[45]              /* b */
#define  MODE_FIN  menu_entries[46]              /* f */
#define  MODE_LOG  menu_entries[47]              /* l */
#define  MODE_SCI  menu_entries[48]              /* s */

#define  KEY_CTRM  mode_buttons[0].value         /* CTL('t') */
#define  KEY_DDB   mode_buttons[1].value         /* CTL('d') */
#define  KEY_FV    mode_buttons[2].value         /* v */
#define  KEY_PMT   mode_buttons[3].value         /* P */
#define  KEY_PV    mode_buttons[4].value         /* p */
#define  KEY_RATE  mode_buttons[5].value         /* CTL('r') */
#define  KEY_SLN   mode_buttons[6].value         /* CTL('s') */
#define  KEY_SYD   mode_buttons[7].value         /* CTL('y') */
#define  KEY_TERM  mode_buttons[8].value         /* T */

#define  KEY_LSFT  mode_buttons[16].value        /* < */
#define  KEY_RSFT  mode_buttons[17].value        /* > */
#define  KEY_16    mode_buttons[18].value        /* [ */
#define  KEY_32    mode_buttons[19].value        /* ] */
#define  KEY_OR    mode_buttons[24].value        /* | */
#define  KEY_AND   mode_buttons[25].value        /* & */
#define  KEY_NOT   mode_buttons[26].value        /* ~ */
#define  KEY_XOR   mode_buttons[27].value        /* ^ */
#define  KEY_XNOR  mode_buttons[28].value        /* n */

#define  KEY_TRIG  mode_buttons[32].value        /* T */
#define  KEY_HYP   mode_buttons[33].value        /* h */
#define  KEY_INV   mode_buttons[34].value        /* i */
#define  KEY_ETOX  mode_buttons[35].value        /* { */
#define  KEY_TTOX  mode_buttons[36].value        /* } */
#define  KEY_YTOX  mode_buttons[37].value        /* y */
#define  KEY_FACT  mode_buttons[38].value        /* ! */
#define  KEY_COS   mode_buttons[40].value        /* CTL('c') */
#define  KEY_SIN   mode_buttons[41].value        /* CTL('s') */
#define  KEY_TAN   mode_buttons[42].value        /* CTL('t') */
#define  KEY_LN    mode_buttons[43].value        /* N */
#define  KEY_LOG   mode_buttons[44].value        /* G */
#define  KEY_RAND  mode_buttons[45].value        /* ? */

#define  BCOLS          8          /* No of columns of buttons. */
#define  BROWS          6          /* No of rows of buttons. */
#define  CTL(n)         n - 96     /* Generate control character value. */
#define  EQUAL(a, b)    !strncmp(a, b, strlen(b))
#define  EXTRA          4          /* Extra useful character definitions. */

#define  FBORDER_SIZE   5          /* Thickness of frame border in pixels. */
#define  FLABEL_SIZE    26         /* Height of frame label in pixels. */

#define  INC            { argc-- ; argv++ ; }
#define  IS_KEY(v, n)   (v == n)
#define  LABEL_LEN      20         /* Length in chars of button labels. */

#ifndef  LINT_CAST
#ifdef   lint
#define  LINT_CAST(arg)  (arg ? 0 : 0)
#else
#define  LINT_CAST(arg)  (arg)
#endif /*lint*/
#endif /*LINT_CAST*/

#define  MAX_DIGITS     40         /* Maximum displayable number of digits. */

/* Maximum number of various graphics pieces. */
#define  MAXFCP         3          /* Max no of frames/canvases/pixwins. */
#define  MAXFONTS       4          /* Maximum number of fonts. */
#define  MAXIMAGES      3          /* Maximum number of button images. */
#define  MAXITEMS       8          /* Maximum number of panel items. */
#define  MAXMENUS       13         /* Maximum number of popup menus. */

#ifndef  MAXLINE
#define  MAXLINE        256        /* Length of character strings. */
#endif /*MAXLINE*/

#define  MAXBASES       4          /* Maximum number of numeric bases. */
#define  MAXDISPMODES   3          /* Maximum number of display modes. */
#define  MAXENTRIES     50         /* Maximum number of menu entries. */
#define  MAXMODES       4          /* Maximum number of calculator modes. */
#define  MAXREGCHARS    40         /* No. of chars. per line in reg. frame. */
#define  MAXREGS        10         /* Maximum number of memory registers. */
#define  MAXSCALES      4          /* Maximum number of predefined scales. */
#define  MAXSTACK       256        /* Parenthese stack size. */
#define  MAXTRIGMODES   3          /* Maximum number of trig. modes. */
#define  MAXVKEYS       5          /* Number of valid keys after an error. */

#define  MCOLS          8          /* Number of columns of "special" keys. */
#define  MROWS          2          /* Number of rows of "special" keys. */
#define  MODEKEYS       MCOLS * MROWS

#define  MAXCOLS        ((v->curwin == FCP_KEY) ? BCOLS : MCOLS)
#define  MAXROWS        ((v->curwin == FCP_KEY) ? BROWS : MROWS)

#ifndef  MIN
#define  MIN(x,y)       ((x) < (y) ? (x) : (y))
#endif /*MIN*/

#define  NOBUTTONS      BROWS * BCOLS

#ifndef  RCNAME
#define  RCNAME         ".calctoolrc"
#endif /*RCNAME*/

#define  SIGRET         void
#define  TITEMS         NOBUTTONS + EXTRA      /* Total definitions. */

#ifndef  TRUE                    /* Boolean definitions. */
#define  TRUE           1
#endif /*TRUE*/

#ifndef  FALSE
#define  FALSE          0
#endif /*FALSE*/

#define  B_COLOR(c)   (v->iscolor ? c : C_BLACK)
#define  W_COLOR(c)   (v->iscolor ? c : C_WHITE)

/* Property sheet row numbers for each item. */
#define  PROW_APP                0    /* Appearance: 2D-look, 3D-look. */
#define  PROW_DISP               1    /* Display: color, monochrome. */
#define  PROW_STYLE              2    /* Style: left-handed, right-handed. */
#define  PROW_BUTTONS            4    /* Apply, Set Default, Reset. */
 
/* Various property sheet column numbers. */
#define  PCOL_ABUT               2    /* Position of the Apply button. */
#define  PCOL_DBUT               14   /* Position of the Set Default button. */
#define  PCOL_JUST               12   /* Justification column for most text. */
#define  PCOL_RBUT               30   /* Position of the Reset button. */
#define  OFFSET                  5    /* Number of pixels between buttons */

typedef  unsigned long  BOOLEAN ;

struct iteminfo {           /* Panel item information record. */
  enum font_type font ;     /* Font type for this panel item. */
  int x ;                   /* X position of item. */
  int y ;                   /* Y position of item. */
} ;

struct button {
  char *str ;               /* Button display string. */
  char *hstr ;              /* Button help string. */
  char value ;              /* Unique button keyboard equivalent. */
  enum op_type opdisp ;     /* Is button selected during operation? */
  enum menu_type mtype ;    /* Type of popup menu (if any). */
  char color ;              /* Color of button portion. */
  void (*func)() ;          /* Function to obey on button press. */
} ;

struct menu {
  char *title ;             /* Menu title. */
  int  total ;              /* Number of menu entries. */
  int  index ;              /* Index into menu string array. */
  int  defval ;             /* Default menu item position (from 1). */
} ;

struct calcVars {                     /* Calctool variables and options. */
  char *appname ;                     /* Application name for resources. */
  char *colstr[CALC_COLORSIZE] ;      /* X color resource strings. */
  char con_names[MAXREGS][MAXLINE] ;  /* Selectable constant names. */
  char cur_op ;                       /* Current arithmetic operation. */
  char current ;                      /* Current button/character pressed. */
  char display[MAXLINE] ;             /* Current calculator display. */
  char disp_state[MAXLINE] ;          /* Display state (hilited/normal). */
  char *exp_posn ;                    /* Position of the exponent sign. */
  char fnum[MAX_DIGITS+1] ;           /* Scratchpad for fixed numbers. */
  char *fontnames[MAXFONTS] ;         /* X resource font names. */
  char fun_names[MAXREGS][MAXLINE] ;  /* Function names from .calctoolrc. */
  char fun_vals[MAXREGS][MAXLINE] ;   /* Function defs from .calctoolrc. */
  char *iconlabel ;                   /* The calctool icon label. */
  char item_text[MAXITEMS][60] ;      /* Pseudo panel item text strings. */
  char old_cal_value ;                /* Previous calculation operator. */
  char opstr[5] ;                     /* Operand string during pending op. */
  char *progname ;                    /* Name of this program. */
  char pstr[5] ;                      /* Current button text string. */
  char *selection ;                   /* Current [Get] selection. */
  char *shelf ;                       /* PUT selection shelf contents. */
  char snum[MAX_DIGITS+1] ;           /* Scratchpad for scientific numbers. */
  char *titleline ;                   /* Value of titleline (if present). */

  int MPcon_vals[MAXREGS][MP_SIZE] ;  /* Selectable constants. */
  int MPdebug ;                       /* If set, debug info. to stderr. */
  int MPerrors ;                      /* If set, output errors to stderr. */
  int MPdisp_val[MP_SIZE] ;           /* Value of the current display. */
  int MPlast_input[MP_SIZE] ;         /* Previous number input by user. */
  int MPmvals[MAXREGS][MP_SIZE] ;     /* Memory register values. */
  int *MPnumstack[MAXSTACK] ;         /* Numeric stack for parens. */
  int MPresult[MP_SIZE] ;             /* Current calculator total value. */
  int MPtresults[3][MP_SIZE] ;        /* Current trigonometric results. */

  enum base_type base ;            /* Current base: BIN, OCT, DEC or HEX. */
  enum but_state bstate[TITEMS] ;  /* Each calctool button state. */

  enum but_state mode_bstate[(MAXMODES-1)*MODEKEYS] ; /* Mode button states. */

  enum fcp_type curwin ;           /* Window current event occured in. */
  enum fcp_type pending_win ;      /* Window that pending op came from. */
  enum mode_type modetype ;        /* Current calculator mode. */
  enum mode_type pending_mode ;    /* Mode for pending op. */
  enum num_type dtype ;            /* Number display mode. */
  enum scale_type scale ;          /* Current calctool display scale. */
  enum trig_type ttype ;           /* Trig. type (deg, grad or rad). */

  long old_sec, old_usec ;         /* Timestamp of previous left mouse down. */
  long sec, usec ;                 /* Timestamp of current left mouse down. */

  struct button temp_buttons[BCOLS] ;   /* To setup "right-handed" version. */
  struct iteminfo items[MAXITEMS] ;     /* Panel items. */

  int rcols[CALC_COLORSIZE] ;           /* Red colormap values. */
  int gcols[CALC_COLORSIZE] ;           /* Green colormap values. */
  int bcols[CALC_COLORSIZE] ;           /* Blue colormap values. */

  int accuracy ;      /* Number of digits precision (Max 9). */
  int bborder ;       /* Number of pixels in button border. */
  int beep ;          /* Indicates whether there is a beep sound on error. */
  int bgap ;          /* Number of pixels between buttons. */
  int bheight ;       /* Number of pixels for button height. */
  int bwidth ;        /* Number of pixels for button width. */
  int column ;        /* Column number of current key/mouse press. */
  int cur_ch ;        /* Current character if keyboard event. */
  int curx ;          /* Current mouse X position. */
  int cury ;          /* Current mouse Y position. */
  int down ;          /* Indicates is a mouse button is down. */
  int error ;         /* Indicates some kind of display error. */
  int event_type ;    /* Type of event being currently processed. */
  int fascent[MAXFONTS] ;  /* Ascent of a char for for each font. */
  int fheight[MAXFONTS] ;  /* Height of a char for for each font. */
  int fwidth[MAXFONTS] ;   /* Width of a char for for each font. */
  int hasicon ;            /* Set if user gave icon name on command line. */
  int histart ;            /* Index of first display character hilited. */
  int hyperbolic ;    /* If set, trig functions will be hyperbolic. */
  int iconic ;        /* Set if window is currently iconic. */
  int iheight ;       /* Initial height of the calctool window. */
  int inverse ;       /* If set, trig and log functions will be inversed. */
  int is_3D ;         /* If set, display with a 3D look on color screens. */
  int iscolor ;       /* Set if this is a color screen. */
  int ismenu ;        /* Set when do_pending called via a popup menu. */
  int isscale ;       /* Set if we were given an initial scale. */
  int issel ;         /* Set if valid [Get] selection. */
  int istitle ;       /* Set if calctool frame has a title. */
  int iwidth ;        /* Initial width of the calctool window. */
  int ix ;            /* Initial X position of the icon. */
  int iy ;            /* Initial Y position of the icon. */
  int key_exp ;       /* Set if entering exponent number. */
  int mheight ;       /* Height of the mode window. */
  int minheight ;     /* Minimum height of the main calctool frame. */
  int minwidth ;      /* Minimum width of the main calctool frame. */
  int monochrome ;    /* If set, display will be in black and white. */
  int mwidth ;        /* Width of the mode window. */
  int ndisplay ;      /* Height of the numerical display. */
  int new_input ;     /* New number input since last op. */
  int noparens ;      /* Count of left brackets still to be matched. */
  int numsptr ;       /* Pointer into the parenthese numeric stack. */
  int oldx ;          /* X position of previous left mouse down. */
  int oldy ;          /* Y position of previous left mouse down. */
  int opsptr ;        /* Pointer into the parentheses operand stack. */
  int opstack[MAXSTACK] ;  /* Stack containing parentheses input. */
  int pending ;            /* Set for command depending on multiple presses. */
  int pending_n ;     /* Offset into function table for pending op. */
  int pending_op ;    /* Arithmetic operation for pending command. */
  int pointed ;       /* Whether a decimal point has been given. */
  int rheight ;       /* Height of the register window. */
  int righthand ;     /* Set if this is a "right-handed" calculator. */
  int row ;           /* Row number of current key/mouse press. */
  int rstate ;        /* Indicates if memory register frame is displayed. */
  int rwidth ;        /* Width of the register window. */
  int show_paren ;    /* Set if we wish to show DISPLAYITEM during parens. */
  int started ;       /* Set just before window is displayed. */
  int theight ;       /* Current main calctool frame height. */
  int toclear ;       /* Indicates if display should be cleared. */
  int tstate ;        /* Indicates current button set being displayed. */
  int twidth ;        /* Current main calctool frame width. */
  int wx ;            /* Initial X position of the window. */
  int wy ;            /* Initial Y position of the window. */
} CalcVars ;

typedef struct calcVars *Vars ;

/* MP definitions. */

#define  C_abs(x)    ((x) >= 0 ? (x) : -(x))
#define  dabs(x)     (double) C_abs(x)
#define  min(a, b)   ((a) <= (b) ? (a) : (b))
#define  max(a, b)   ((a) >= (b) ? (a) : (b))
#define  dmax(a, b)  (double) max(a, b)
#define  dmin(a, b)  (double) min(a, b)

BOOLEAN ibool                P((double)) ;

char *adjust_string          P((char *, int)) ;
char  button_color           P((int)) ;
char *button_str             P((int)) ;
char  button_value           P((int)) ;
char *convert                P((char *)) ;
char *get_def_menu_str       P((enum menu_type)) ;
char *get_resource           P((enum res_type)) ;
char *help_button_str        P((int)) ;
char *make_eng_sci           P((int *)) ;
char *make_fixed             P((int *, int)) ;
char *make_number            P((int *)) ;
char *set_bool               P((int)) ;

double mppow_di              P((double *, int *)) ;
double mppow_ri              P((float *, int *)) ;
double setbool               P((BOOLEAN)) ;

enum but_state button_bstate P((int)) ;
enum menu_type button_mtype  P((int)) ;
enum op_type button_opdisp   P((int)) ;

unsigned short *get_but_data P(()) ;

int char_val               P((int)) ;
int do_menu                P((enum fcp_type, int, enum menu_type)) ;
int get_bool_resource      P((enum res_type, int *)) ;
int get_font               P((int, enum font_type)) ;
int get_hilite_index       P((char *, int, int)) ;
int get_index              P((int)) ;
int get_int_resource       P((enum res_type, int *)) ;
int get_menu_def           P((int)) ;
int get_menu_pos           P((enum menu_type, int)) ;
int get_row_col            P((int *, int *)) ;
int get_str_resource       P((enum res_type, char *)) ;
int get_strwidth           P((enum font_type, char *)) ;
int is_dblclick            P(()) ;
int main                   P((int, char **)) ;
int modewin_pinned         P(()) ;
int set_min_colors         P(()) ;

void add_3D_corner         P((enum fcp_type, enum scale_type,
                              enum corner_type, int, int, int, int, int)) ;
void add_3D_look           P((enum fcp_type, int, int, int, int,
                              enum scale_type, enum but_state,
                              enum menu_type, int, int)) ;
void beep                  P(()) ;
void check_args            P(()) ;
void check_ow_beep         P(()) ;
void clear_display         P(()) ;
void clear_hilite          P(()) ;
void color_area            P((enum fcp_type, int, int, int, int, int)) ;
void create_con_fun_menu   P((enum menu_type, int)) ;
void create_menu           P((enum menu_type, int)) ;
void do_accuracy           P(()) ;
void do_ascii              P(()) ;
void do_base               P(()) ;
void do_business           P(()) ;
void do_calc               P(()) ;
void do_calctool           P((int, char **)) ;
void do_canvas_resize      P(()) ;
void do_clear              P(()) ;
void do_constant           P(()) ;
void do_delete             P(()) ;
void do_display_adjust     P(()) ;
void do_display_select     P(()) ;
void do_exchange           P(()) ;
void do_expno              P(()) ;
void do_factorial          P((int *, int *)) ;
void do_frame              P(()) ;
void do_function           P(()) ;
void do_help               P(()) ;
void do_immed              P(()) ;
void do_keys               P(()) ;
void do_memory             P(()) ;
void do_mode               P(()) ;
void do_mouse_dragging     P(()) ;
void do_mouse_left_down    P(()) ;
void do_mouse_left_up      P(()) ;
void do_mouse_right_down   P(()) ;
void do_none               P(()) ;
void do_number             P(()) ;
void do_numtype            P(()) ;
void do_paren              P(()) ;
void do_pending            P(()) ;
void do_point              P(()) ;
void do_portion            P(()) ;
void do_repaint            P(()) ;
void do_shift              P(()) ;
void do_sto_rcl            P(()) ;
void do_trig               P(()) ;
void do_trigtype           P(()) ;
void draw_button           P((enum fcp_type, int, int, enum but_state)) ;
void draw_def_menu         P((enum menu_type, int)) ;
void draw_image            P((enum fcp_type, int, int, int, enum but_state)) ;
void draw_line             P((enum fcp_type, int, int, int, int, int)) ;
void draw_menu_image       P((enum fcp_type, int, int, enum but_state)) ;
void draw_menu_stencil     P((enum fcp_type, int, int, int, enum but_state)) ;
void draw_stencil          P((enum fcp_type, int, int, int, enum but_state)) ;
void draw_text             P((int, int, enum fcp_type,
                                        enum font_type, int, char *)) ;
void get_but_corners       P((enum but_state, unsigned short *)) ;
void get_display           P(()) ;
void get_font_scale        P((char *)) ;
void get_frame_size        P((enum fcp_type, int *, int *, int *, int *)) ;
void get_key_val           P((char *, char *)) ;
void get_label             P((int, int *, int *)) ;
void get_options           P((int, char **)) ;
void getparam              P((char *, char **, char *)) ;
void get_rcfile            P((char *)) ;
void grey_area             P((enum fcp_type, int, int, int, int)) ;
void grey_buttons          P((enum base_type)) ;
void handle_menu_selection P((int, int, int)) ;
void handle_resize         P((int, int)) ;
void handle_selection      P(()) ;
void hilite_char           P((int, int, int, int, int)) ;
void initialise            P(()) ;
void init_args             P(()) ;
void init_cmdline_opts     P(()) ;
void init_fonts            P((enum scale_type)) ;
void init_frame_sizes      P(()) ;
void init_options          P(()) ;
void init_other_dims       P(()) ;
void init_panel_item_sizes P(()) ;
void init_size             P(()) ;
void init_text             P(()) ;
void init_vars             P(()) ;
void init_Xvars            P((int *, char **)) ;
void key_init              P(()) ;
void load_corners          P(()) ;
void load_deskset_defs     P(()) ;
void load_resources        P(()) ;
void make_button           P((enum but_state, enum scale_type, int, int)) ;
void make_buttons          P(()) ;
void make_canvas           P((int, int, int, int, int)) ;
void make_frames           P(()) ;
void make_items            P(()) ;
void make_menus            P(()) ;
void make_modewin          P((int, int, int, int)) ;
void make_registers        P(()) ;
void make_subframes        P(()) ;
void MPstr_to_num          P((char *, enum base_type, int *)) ;
void paren_disp            P((int)) ;
void process_event         P((int)) ;
void process_item          P((int)) ;
void process_parens        P((int)) ;
void process_stack         P((int, int, int)) ;
void process_str           P((char *)) ;
void push_num              P((int *)) ;
void push_op               P((int)) ;
void put_resource          P((enum res_type, char *)) ;
void read_rcfiles          P(()) ;
void read_resources        P(()) ;
void read_str              P((char **, char *)) ;
void reset_prop_vals       P(()) ;
void save_cmdline          P((int, char **)) ;
void save_pending_values   P((int)) ;
void save_resources        P(()) ;
void set_def_vals          P(()) ;
void set_frame_size        P((enum fcp_type, int, int, int, int)) ;
void set_ins_key           P(()) ;
void set_item              P((enum item_type, char *)) ;
void set_prop_options      P((int, int, int)) ;
void set_title             P((enum fcp_type, char *)) ;
void show_ascii_frame      P(()) ;
void show_display          P((int *)) ;
void show_help             P((char *)) ;
void srand48               P(()) ;
void start_popup           P((enum menu_type)) ;
void start_tool            P(()) ;
void switch_hands          P((int)) ;
void usage                 P((char *)) ;
void win_display           P((enum fcp_type, int)) ;
void write_cmdline         P(()) ;
void write_rcfile          P((enum menu_type, int, int, char *, char *)) ;
void write_resources       P(()) ;

/* MP routines not found in the Brent FORTRAN package. */
void mpacos                P((int *, int *)) ;
void mpacosh               P((int *, int *)) ;
void mpasinh               P((int *, int *)) ;
void mpatanh               P((int *, int *)) ;
void mplog10               P((int *, int *)) ;

/* Brent MP routines in mp.c. */
int mpcmpi        P((int *, int *)) ;
int mpcmpr        P((int *, float *)) ;
int mpcomp        P((int *, int *)) ;
int pow_ii        P((int *, int *)) ;
 
int mpeq          P((int *, int *)) ;
int mpge          P((int *, int *)) ;
int mpgt          P((int *, int *)) ;
int mple          P((int *, int *)) ;
int mplt          P((int *, int *)) ;

void mpabs        P((int *, int *)) ;
void mpadd        P((int *, int *, int *)) ;
void mpadd2       P((int *, int *, int *, int *, int *)) ;
void mpadd3       P((int *, int *, int *, int *, int *)) ;
void mpaddi       P((int *, int *, int *)) ;
void mpaddq       P((int *, int *, int *, int *)) ;
void mpart1       P((int *, int *)) ;
void mpasin       P((int *, int *)) ;
void mpatan       P((int *, int *)) ;
void mpcdm        P((double *, int *)) ;
void mpchk        P((int *, int *)) ;
void mpcim        P((int *, int *)) ;
void mpclr        P((int *, int *)) ;
void mpcmd        P((int *, double *)) ;
void mpcmf        P((int *, int *)) ;
void mpcmi        P((int *, int *)) ;
void mpcmim       P((int *, int *)) ;
void mpcmr        P((int *, float *)) ;
void mpcos        P((int *, int *)) ;
void mpcosh       P((int *, int *)) ;
void mpcqm        P((int *, int *, int *)) ;
void mpcrm        P((float *, int *)) ;
void mpdiv        P((int *, int *, int *)) ;
void mpdivi       P((int *, int *, int *)) ;
void mperr        P(()) ;
void mpexp        P((int *, int *)) ;
void mpexp1       P((int *, int *)) ;
void mpext        P((int *, int *, int *)) ;
void mpgcd        P((int *, int *)) ;
void mpln         P((int *, int *)) ;
void mplns        P((int *, int *)) ;
void mpmaxr       P((int *)) ;
void mpmlp        P((int *, int *, int *, int *)) ;
void mpmul        P((int *, int *, int *)) ;
void mpmul2       P((int *, int *, int *, int *)) ;
void mpmuli       P((int *, int *, int *)) ;
void mpmulq       P((int *, int *, int *, int *)) ;
void mpneg        P((int *, int *)) ;
void mpnzr        P((int *, int *, int *, int *)) ;
void mpovfl       P((int *)) ;
void mppi         P((int *)) ;
void mppwr        P((int *, int *, int *)) ;
void mppwr2       P((int *, int *, int *)) ;
void mprec        P((int *, int *)) ;
void mproot       P((int *, int *, int *)) ;
void mpset        P((int *, int *, int *)) ;
void mpsin        P((int *, int *)) ;
void mpsin1       P((int *, int *, int *)) ;
void mpsinh       P((int *, int *)) ;
void mpsqrt       P((int *, int *)) ;
void mpstr        P((int *, int *)) ;
void mpsub        P((int *, int *, int *)) ;
void mptanh       P((int *, int *)) ;
void mpunfl       P((int *)) ;
