#ifndef lint
static char sccsid[] = "@(#)calctool.c 1.8 93/07/08 Copyr 1987 Sun Micro" ;
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
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <math.h>
#include "patchlevel.h"
#include "color.h"
#include "calctool.h"

time_t time() ;

double max_fix[4] = {
          6.871947674e+10, 3.245185537e+32,
          1.000000000e+36, 2.230074520e+43
} ;

extern char *base_str[] ;       /* Strings for each base value. */
extern char *cmdstr[] ;         /* Strings for each command line option. */
extern char *dtype_str[] ;      /* Strings for each display mode value. */
extern char *mode_str[] ;       /* Strings for each mode value. */
extern char *mstrs[] ;          /* Mode titles for the popup panel. */
extern char *opts[] ;           /* Command line option strings. */
extern char *ttype_str[] ;      /* Strings for each trig type value. */
extern char *vstrs[] ;          /* Various strings. */

/*  The tables below gives the radius for each corner, and the X and Y
 *  offsets within the corner server image for that scale.
 */

int cornerR[MAXSCALES] = { 8, 8,  10, 12 } ;
int cornerX[MAXSCALES] = { 0, 32, 0,  32 } ;
int cornerY[MAXSCALES] = { 0, 0,  32, 32 } ;


int bsizes[MAXSCALES] = { 6,  8,  12, 16 } ;      /* Border sizes. */
int fsizes[MAXSCALES] = { 10, 12, 14, 19 } ;      /* Font scales. */
int gsizes[MAXSCALES] = { 6,  8,  10, 12 } ;      /* Gap sizes. */
int msizes[MAXSCALES] = { 7,  7,  11, 13 } ;      /* Menu glyph sizes. */

/*  The current scale is dependent upon the width and height of the calctool
 *  buttons. These tables give the minimum dimension in pixels for the
 *  width and height for each scale.
 */

int scaleH[MAXSCALES] = { 22, 28, 38, 48 } ;
int scaleW[MAXSCALES] = { 44, 48, 68, 88 } ;

char digits[] = "0123456789ABCDEF" ;
int basevals[4] = { 2, 8, 10, 16 } ;

/* Length of display in characters for each base. */
int disp_length[4] = { 40, 15, 12, 12 } ;

int left_pos[BCOLS]  = { 7, 6, 4, 5, 0, 1, 2, 3 } ;  /* Left positions. */
int right_pos[BCOLS] = { 4, 5, 6, 7, 2, 3, 1, 0 } ;  /* "Right" positions. */

/* Various string values read/written as X resources. */

char *Rbstr[MAXBASES]     = { "BIN", "OCT", "DEC", "HEX" } ;
char *Rdstr[MAXDISPMODES] = { "ENG", "FIX", "SCI" } ;
char *Rmstr[MAXMODES]     = { "BASIC", "FINANCIAL", "LOGICAL", "SCIENTIFIC" } ;
char *Rtstr[MAXTRIGMODES] = { "DEG", "GRAD", "RAD" } ;

/* Valid keys when an error condition has occured. */
/*                            MEM  KEYS clr     QUIT REDRAW */
char validkeys[MAXVKEYS]  = { 'm', 'k', '\177', 'q', '\f' } ;

Vars v ;            /* Calctool variables and options. */

char menu_entries[MAXENTRIES] ;

struct menu cmenus[MAXMENUS] = {                 /* Calculator menus. */
/*      title     total index defval                           */
  { (char *) NULL, 10,    0,    4  /* 2 places */    },    /* ACC */
  { (char *) NULL,  4,   35,    4  /* Decimal  */    },    /* BASE TYPE */
  { (char *) NULL, 10,    0,    2  /* Con. 0   */    },    /* CON */
  { (char *) NULL, 10,   10,    2  /* Reg. 0   */    },    /* EXCH */
  { (char *) NULL, 10,    0,    2  /* Fun. 0   */    },    /* FUN */
  { (char *) NULL, 15,   20,    2  /* Shift 1  */    },    /* LSHF */
  { (char *) NULL,  4,   45,    2  /* Basic    */    },    /* MODE */
  { (char *) NULL,  3,   39,    3  /* Fixed    */    },    /* NUM TYPE */
  { (char *) NULL, 10,   10,    2  /* Reg. 0   */    },    /* RCL */
  { (char *) NULL, 15,   20,    2  /* Shift 1  */    },    /* RSHF */
  { (char *) NULL, 10,   10,    2  /* Reg. 0   */    },    /* STO */
  { (char *) NULL,  3,   42,    2  /* Degrees  */    },    /* TRIG TYPE */
  { (char *) NULL,  1,   49,    2, /* Properties. */ }     /* PROPS. */
} ;

/*  This table shows the keyboard values that are currently being used:
 *
 *           | a b c d e f g h i j k l m n o p q r s t u v w x y z
 *-------------+--------------------------------------------------
 *  Control: | a   c d   f   h i     l m         r s t u       y
 *  Lower:   | a b c d e f   h i   k   m n   p q r s     v   x y
 *  Upper:   | A B C D E F G           M N   P Q R S T       X
 *  Numeric: | 0 1 2 3 4 5 6 7 8 9
 *  Other:   | @ . + - * / = % ( ) # < > [ ] { } | & ~ ^ ? ! \177
 *----------------------------------------------------------------
 */

/* Calculator button values. */

struct button buttons[TITEMS] = {
/*     str       hstr   value  opdisp   menutype   color       func */

/* Row 1. */
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_HEXDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_HEXDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_HEXDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_ADJUST,   do_clear     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_PORTION,  do_portion   },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_PORTION,  do_portion   },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_BASE, C_MAINMODE, do_pending   },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NUM,  C_MAINMODE, do_pending   },

/* Row 2. */
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_HEXDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_HEXDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_HEXDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_ADJUST,   do_delete    },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_PORTION,  do_portion   },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_PORTION,  do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FUNC,     do_keys      },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_MODE, C_MAINMODE, do_pending   },

/* Row 3. */
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_ARITHOP,  do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_ARITHOP,  do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_ARITHOP,  do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_ACC,  C_FUNC,     do_pending   },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_FUNC,     do_memory    },

/* Row 4. */
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_ARITHOP,  do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_ARITHOP,  do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_ARITHOP,  do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_CON,  C_FUNC,     do_pending   },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_FUN,  C_FUNC,     do_pending   },

/* Row 5. */
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_ARITHOP,  do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_ARITHOP,  do_paren     },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_ARITHOP,  do_paren     },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_RCL,  C_FUNC,     do_pending   },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_STO,  C_FUNC,     do_pending   },

/* Row 6. */
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_number    },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_DECDIG,   do_point     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_ARITHOP,  do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_ARITHOP,  do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_ARITHOP,  do_expno     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_ARITHOP,  do_ascii     },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_EXCH, C_FUNC,     do_pending   },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FUNC,     do_frame     },

/* Extra definitions. */

{ "x   ", (char *) NULL, '*',      OP_SET,   M_NONE, C_WHITE,   do_calc        },
{ "    ", (char *) NULL, CTL('m'), OP_CLEAR, M_NONE, C_WHITE,   do_calc        },
{ "    ", (char *) NULL, 'Q',      OP_CLEAR, M_NONE, C_WHITE,   do_frame       },
{ "    ", (char *) NULL, '\f',     OP_NOP,   M_NONE, C_WHITE,   do_repaint     },
} ;

struct button mode_buttons[(MAXMODES-1) * MODEKEYS] = {
/*  str           value opdisp   menutype color       func */

/* Financial. */
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FIN,      do_business  },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FIN,      do_business  },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FIN,      do_business  },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FIN,      do_business  },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FIN,      do_business  },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FIN,      do_business  },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FIN,      do_business  },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FIN,      do_business  },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_FIN,      do_business  },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },

/* Logical. */
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_LSHF, C_PLOGICAL, do_pending   },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_RSHF, C_PLOGICAL, do_pending   },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_PLOGICAL, do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_PLOGICAL, do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_BLOGICAL, do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_BLOGICAL, do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_BLOGICAL, do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_BLOGICAL, do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_BLOGICAL, do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },

/* Scientific. */
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_TRIG, C_TRIGMODE, do_pending   },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_TRIGMODE, do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_TRIGMODE, do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_SCI,      do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_SCI,      do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_SET,   M_NONE, C_SCI,      do_calc      },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_SCI,      do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_TRIGCOL,  do_trig      },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_TRIGCOL,  do_trig      },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_TRIGCOL,  do_trig      },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_SCI,      do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_SCI,      do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_CLEAR, M_NONE, C_SCI,      do_immed     },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
{ (char *) NULL,  (char *) NULL, NULL, OP_NOP,   M_NONE, C_BACK,     do_none      },
} ;


void
do_calctool(argc, argv)
int argc ;
char **argv ;
{
  char *ptr ;
  int i ;

  v->progname = argv[0] ;     /* Save programs name. */
  v->appname  = NULL ;
  init_cmdline_opts() ;       /* Initialise command line option strings. */

  if ((ptr = strrchr(argv[0], '/')) != NULL)
    read_str(&v->appname, ptr+1) ;
  else read_str(&v->appname, argv[0]) ;

/*  Search through all the command line arguments, looking for -name.
 *  If it's present, then this name with be used, when looking for X resources
 *  for this application. When the rest of the command line arguments are
 *  checked later on, then the -name argument (if found) is ignored.
 */

  for (i = 0; i < argc; i++)
    if (EQUAL(argv[i], cmdstr[(int) CMD_NAME]))
      {
        if ((i+1) > argc) usage(v->progname) ;
        read_str(&v->appname, argv[i+1]) ;
        break ;
      }

  calc_colorsetup(v->rcols, v->gcols, v->bcols) ;  /* Setup default colors. */

  init_text() ;               /* Setup text strings depending upon language. */
  init_vars() ;               /* Setup default values for variables. */
  key_init() ;                /* Determine numeric function keys. */
  load_resources() ;          /* Get resources from various places. */
  read_resources() ;          /* Read resources from merged database. */
  get_options(argc, argv) ;   /* Get command line arguments. */
  read_rcfiles() ;            /* Read .calctoolrc's files. */
  make_frames() ;             /* Create calctool window frames. */
  init_size() ;               /* Work out the initial size of calctool. */
  make_subframes() ;          /* Create panels and canvases. */
  load_corners() ;            /* Create images for button corners. */

  v->shelf      = NULL ;      /* No selection for shelf initially. */
  v->noparens   = 0 ;         /* No unmatched brackets initially. */
  v->opsptr     = 0 ;         /* Nothing on the parentheses op stack. */
  v->numsptr    = 0 ;         /* Nothing on the parenthese numeric stack. */
  v->pending    = 0 ;         /* No initial pending command. */
  v->tstate     = 0 ;         /* Button values displayed first. */
  v->hyperbolic = 0 ;         /* Normal trig functions initially. */
  v->inverse    = 0 ;         /* No inverse functions initially. */
  v->down       = 0 ;         /* No mouse presses initially. */

  srand48((long) time((time_t *) 0)) ;   /* Seed random number generator. */

  make_items() ;              /* Create server images and fir frames. */
  if (v->iwidth == -1 && v->iheight == -1) v->isscale = TRUE ; 
  make_buttons() ;            /* Generate calctool buttons at correct size. */
  do_clear() ;                /* Initialise and clear display. */

  if (v->rstate == TRUE)      /* Show the memory register window? */
    {
      make_registers() ;
      if (!v->iconic) win_display(FCP_REG, TRUE) ;
    }
  if (v->modetype != BASIC)         /* Show the mode window? */
    {
      set_title(FCP_MODE, mstrs[(int) v->modetype]) ;
      set_item(MODEITEM, mode_str[(int) v->modetype]) ;
      if (!v->iconic) win_display(FCP_MODE, TRUE) ;
    }
  if (v->righthand)                 /* Display a right-handed calculator. */
    switch_hands(v->righthand) ;

  show_display(v->MPdisp_val) ;     /* Output in correct display mode. */
  write_cmdline() ;                 /* Setup calctool command line. */
  start_tool() ;                    /* Display the calculator. */
}


/* Calctools' customised math library error-handling routine. */

doerr(errmes)
char *errmes ;
{
  if (!v->started) return ;
  STRCPY(v->display, errmes) ;
  set_item(DISPLAYITEM, v->display) ;
  v->error = 1 ;
  if (v->beep == TRUE) beep() ;
  set_item(OPITEM, vstrs[(int) V_CLR]) ;
}


/* Default math library exception handling routine. */

/*ARGSUSED*/
int
matherr(exc)
struct exception *exc ;
{
  doerr(vstrs[(int) V_ERROR]) ;
}
