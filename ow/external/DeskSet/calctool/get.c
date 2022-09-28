#ifndef lint
static char sccsid[] = "@(#)get.c 1.7 93/03/25 Copyr 1987 Sun Micro";
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
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <pwd.h>
#include "patchlevel.h"
#include "color.h"
#include "calctool.h"
#include "extern.h"


char *
convert(line)              /* Convert .calctoolrc line to ascii values. */
char *line ;               /* Input line to be converted. */
{
  static char output[MAXLINE] ;   /* Converted output record. */
  int ctrl = 0       ;     /* Set if we are processing a control character. */
  int i ;                  /* Position within input line. */
  int len ;
  int n = 0 ;              /* Position within output line. */

  len = strlen(line) ;
  for (i = 0; i < len; i++)
    {
           if (line[i] == ' ') continue ;
      else if (line[i] == '\\') ctrl = 1 ;
      else if (ctrl)
        {
          output[n++] = CTL(line[i]) ;
          ctrl = 0 ;
        }
      else output[n++] = line[i] ;
    }
  output[n] = '\0' ;
  return(output) ;
}


int
get_bool_resource(rtype, boolval)   /* Get boolean resource from database. */
enum res_type rtype ;
int *boolval ;
{
  char *val, tempstr[MAXLINE] ;
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


void
get_font_scale(s)
char *s ;
{
       if (EQUAL(s, sstrs[(int) S_SMALL]))      v->scale = S_SMALL ;
  else if (EQUAL(s, sstrs[(int) S_MEDIUM]))     v->scale = S_MEDIUM ; 
  else if (EQUAL(s, sstrs[(int) S_LARGE]))      v->scale = S_LARGE ;
  else if (EQUAL(s, sstrs[(int) S_EXTRALARGE])) v->scale = S_EXTRALARGE ;
}


int
get_int_resource(rtype, intval)   /* Get integer resource from database. */
enum res_type rtype ;
int *intval ;
{
  char *val ;
 
  if ((val = get_resource(rtype)) == NULL) return(0) ;
  *intval = atoi(val) ;
  return(1) ;
}


/* Get keyboard equivalent from first character of localised string. */

void
get_key_val(val, str)
char *val, *str ;
{
  *val = str[0] ;
}


void
get_options(argc, argv)      /* Extract command line options. */
int argc ;
char *argv[] ;
{
  char next[MAXLINE] ;       /* The next command line parameter. */

  INC ;
  while (argc > 0)
    {
      if (argv[0][0] == '-' || argv[0][0] == '+')
        {
          if (argv[0][2] != '\0') goto toolarg ;
          switch (argv[0][1])
            {
              case '2' : v->is_3D = 0 ;    /* Start with a 2D look. */
                         break ;
              case '3' : v->is_3D = 1 ;    /* Start with a 3D look. */
                         break ;
              case 'D' : v->MPdebug = TRUE ;   /* MP debug info. to stderr. */
                         break ;
              case 'E' : v->MPerrors = TRUE ;  /* MP errors to stderr. */
                         break ;
              case 'a' : INC ;
                         getparam(next, argv, opts[(int) O_ACCVAL]) ;
                         v->accuracy = atoi(next) ;
                         if (v->accuracy < 0 || v->accuracy > 9)
                           {
                             FPRINTF(stderr, opts[(int) O_ACCRANGE],
                                     v->progname) ;
                             v->accuracy = 2 ;
                           }
                         break ;
              case 'c' : v->monochrome = 0 ;   /* Display in color. */
                         break ;
              case 'l' : v->righthand = 0 ;    /* "Left-handed" version. */
                         break ;
              case 'm' : v->monochrome = 1 ;   /* Display in black and white. */
                         break ;
              case 'r' : v->righthand = 1 ;    /* "Right-handed" version */
                         break ;
              case '?' :
              case 'v' : usage(v->progname) ;
                         break ;
              default  :
              toolarg  :             /* Pick up generic tool arguments. */

                         if (EQUAL(argv[0], cmdstr[(int) CMD_NAME]))
                           {
                             if (argc < 2) usage(v->progname) ;
                             argc -= 2 ;
                             argv += 2 ;
                             continue ;
                           }

/*  Check to see if the user has supplied a -Wn or a +Wn argument. The first
 *  signifies no titleline required, and the latter indicates the opposite.
 */

                       if (!strncmp(&argv[0][1], cmdstr[(int) CMD_WN], 2))
                         {
                           v->istitle = (argv[0][0] == '-') ? FALSE : TRUE ;
                           argc-- ;
                           argv++ ;
                           continue ;
                         }

                       usage(v->progname) ;
            }
          INC ;
        }
      else INC ;
    }
  check_args() ;
}


void
getparam(s, argv, errmes)
char *s, *argv[], *errmes ;
{
  if (*argv != NULL && argv[0][0] != '-') STRCPY(s, *argv) ;
  else
    { 
      FPRINTF(stderr, mess[(int) MESS_PARAM], v->progname, errmes) ;
      exit(1) ;                        
    }                                  
}


void
get_rcfile(name)          /* Read .calctoolrc file. */
char *name ;
{
  char line[MAXLINE] ;    /* Current line from the .calctoolrc file. */
  char tmp[MAXLINE] ;     /* Used to extract definitions. */
  double cval ;           /* Current constant value being converted. */
  enum base_type base ;   /* Saved current base value. */
  int i ;                 /* Index to constant or function array. */
  int isval ;             /* Set to 'c' or 'f' for convertable line. */
  int len, n ;
  FILE *rcfd ;            /* File descriptor for calctool rc file. */

  if ((rcfd = fopen(name, "r")) == NULL) return ;

/*  Process the .calctoolrc file. There are currently four types of
 *  records to look for:
 *
 *  1) Those starting with a hash in the first column are comments.
 *
 *  2) Lines starting with 'c' or 'C' in the first column are
 *     definitions for constants. The cC is followed by a digit in
 *     the range 0-9, then a space. This is followed by a number
 *     in fixed or scientific notation. Following this is an optional
 *     comment, which if found, will be used in the popup menu for
 *     the constants. If the comment is present, there must be at
 *     least one space between this and the preceding number.
 *
 *  3) Those starting with a 'f' or a 'F' in the first column are
 *     definitions for functions. The fF is followed by a digit in
 *     the range 0-9, then a space. This is followed by a function
 *     definition. Following this is an optional comment, which if
 *     found, will be used in the popup menu for the functions.
 *     If the comment is present, there must be at least one space
 *     between this and the preceding function definition.
 *
 *  4) Lines starting with a 'r' or a 'R' in the first column are
 *     definitions for the initial contents of the calculators
 *     memory registers. The rR is followed by a digit in the
 *     range 0-9, then a space. This is followed by a number in
 *     fixed or scientific notation. The rest of the line is ignored.
 *
 *  All other lines are ignored.
 *
 *  Two other things to note. There should be no embedded spaces in
 *  the function definitions, and whenever a backslash is found, that
 *  and the following character signify a control character, for
 *  example \g would be ascii 7.
 */

  while (fgets(line, MAXLINE, rcfd) != NULL)
    {
      isval = 0 ;
           if (line[0] == 'c' || line[0] == 'C') isval = 'c' ;
      else if (line[0] == 'f' || line[0] == 'F') isval = 'f' ;
      else if (line[0] == 'r' || line[0] == 'R') isval = 'r' ;
      if (isval)
        if (line[1] >= '0' && line[1] <= '9' && line[2] == ' ')
          {
            i = char_val(line[1]) ;
            if (isval == 'c')
              {
                n = sscanf(&line[3], "%lf", &cval) ;
                if (n == 1) MPstr_to_num(&line[3], DEC, v->MPcon_vals[i]) ;
              }
            else if (isval == 'f')
              {
                SSCANF(&line[3], "%s", tmp) ;      
                STRCPY(v->fun_vals[i], convert(tmp)) ;
              }
            else if (isval == 'r')
              {
                n = sscanf(&line[3], "%lf", &cval) ;
                if (n == 1) MPstr_to_num(&line[3], DEC, v->MPmvals[i]) ;
                continue ;
              }
            len = strlen(line) ;
            for (n = 3; n < len; n++)
              if (line[n] == ' ' || line[n] == '\n')
                {
                  while (line[n] == ' ') n++ ;
                  line[strlen(line)-1] = '\0' ;
                  if (isval == 'c')
                    {
                      base = v->base ;
                      v->base = DEC ;
                      STRCPY(tmp, make_number(v->MPcon_vals[i])) ;
                      v->base = base ;
                      SPRINTF(v->con_names[i], "%1d: %s [%s]",
                              i, tmp, &line[n]) ;
                    }
                  else
                    SPRINTF(v->fun_names[i], "%1d: %s [%s]",
                            i, tmp, &line[n]) ;
                  break ;
                }
          }
    }
  FCLOSE(rcfd) ;
}


int
get_str_resource(rtype, strval)   /* Get a string resource from database. */
enum res_type rtype ;
char *strval ;
{
  char *val ;
  int i, len ;

  if ((val = get_resource(rtype)) == NULL) return(0) ;
  STRCPY(strval, val) ;
  len = strlen(strval) ;
  for (i = 0; i < len; i++)
    if (islower(strval[i])) strval[i] = toupper(strval[i]) ;
  return(1) ;
}


void
init_size()    /* Work out the initial size of calctool. */
{
  int validsize = FALSE ;
  int bminh, bminw, i, len ;
  int s ;       /* Current scale. */

  bminh = bminw = 0 ;
  if (v->iwidth != -1 && v->iheight != -1) /* -Ws, -size or -geometry ? */
    {
      v->scale    = S_SMALL ;
      s           = (int) v->scale ;
      v->bborder  = bsizes[s] ;
      v->bgap     = gsizes[s] ;
      v->ndisplay = v->iheight / 4 ;
      if (v->ndisplay % 2) v->ndisplay-- ;

      bminw = (v->iwidth - (2 * v->bborder) - ((BCOLS-1) * v->bgap)) / BCOLS ;
      if (bminw % 2) bminw-- ;

      bminh = ((v->iheight - v->ndisplay) -
              (2 * v->bborder) - ((BROWS-1) * v->bgap)) / BROWS ;
      if (bminh % 2) bminh-- ;

      if (bminw > scaleW[(int) S_SMALL] && bminh > scaleH[(int) S_SMALL])
        validsize = TRUE ;
    }

  if (validsize == TRUE)
    {
      v->bheight = bminh ;
      v->bwidth  = bminw ;

/*  Depending upon the button width and height, we determine what scale 
 *  we are currently displaying, and use the appropriate corners to draw 
 *  the three server images. 
 */ 

      for (i = 0; i < MAXSCALES; i++) 
        if (v->bwidth > scaleW[i] && v->bheight > scaleH[i])
          v->scale = (enum scale_type) i ; 

      init_fonts(v->scale) ;
    }
  else
    {
      init_fonts(v->scale) ;

/* Determine maximum button label width (main plus mode windows). */

      for (i = 0; i < TITEMS; i++)
        if ((len = get_strwidth(NFONT, buttons[i].str)) > bminw)
          bminw = len ;

      for (i = 0; i < ((MAXMODES-1) * MODEKEYS); i++)
        if ((len = get_strwidth(NFONT, mode_buttons[i].str)) > bminw)
          bminw = len ;
      bminw += 10 ;       /* Add a small fudge factor to maxium width. */

      s = (int) v->scale ;
      v->bheight  = (bminh > scaleH[s]) ? bminh : scaleH[s] ;
      v->bwidth   = (bminw > scaleW[s]) ? bminw : scaleW[s] ;
      v->bborder  = bsizes[s] ;
      v->bgap     = gsizes[s] ;
      v->ndisplay = ((2 * v->bborder) + ((BROWS-1) * v->bgap) +
                    (BROWS * v->bheight)) / 3 ;
      if (v->ndisplay % 2) v->ndisplay-- ;
    }

  init_other_dims() ;
  init_panel_item_sizes() ;
}


void
init_vars()    /* Setup default values for various variables. */
{
  int acc, i, n, s, size ;

  set_def_vals() ;            /* Set defaults for property sheet items. */
  v->accuracy      = 2 ;      /* Initial accuracy. */
  v->base          = DEC ;    /* Initial base. */
  v->dtype         = FIX ;    /* Initial number display mode. */
  v->ttype         = DEG ;    /* Initial trigonometric type. */
  v->modetype      = BASIC ;  /* Initial calculator mode. */
  v->rstate        = 0 ;      /* No memory register frame display initially. */
  v->iconic        = FALSE ;  /* Calctool not iconic by default. */
  v->MPdebug       = FALSE ;  /* No debug info by default. */
  v->MPerrors      = FALSE ;               /* No error information. */
  acc              = MAX_DIGITS + 12 ;     /* MP internal accuracy. */
  size             = MP_SIZE ;
  mpset(&acc, &size, &size) ;

  v->iheight = v->iwidth = -1 ;   /* To signify no initial size. */
  v->hasicon     = FALSE ;        /* Use standard calctool icon by default. */
  v->isscale     = FALSE ;        /* No initial scale given. */
  v->istitle     = TRUE ;         /* Show a frame title by default. */
  v->beep        = TRUE ;         /* Beep on error by default. */
  v->scale       = S_MEDIUM ;
  v->error       = 0 ;            /* No calculator error initially. */
  v->key_exp     = 0 ;            /* Not entering an exponent number. */
  v->pending_op  = 0 ;            /* No pending arithmetic operation. */
  v->titleline   = NULL ;         /* No User supplied title line. */

  read_str(&v->iconlabel, lstrs[(int) L_LCALC]) ;  /* Default icon label. */

  MPstr_to_num("0.621", DEC, v->MPcon_vals[0]) ;  /* kms/hr <=> miles/hr. */
  MPstr_to_num("1.4142135623", DEC, v->MPcon_vals[1]) ;  /* square root of 2 */
  MPstr_to_num("2.7182818284", DEC, v->MPcon_vals[2]) ;  /* e */
  MPstr_to_num("3.1415926535", DEC, v->MPcon_vals[3]) ;  /* pi */
  MPstr_to_num("2.54",         DEC, v->MPcon_vals[4]) ;  /* cms <=> inch. */
  MPstr_to_num("57.295779513", DEC, v->MPcon_vals[5]) ;  /* degrees/radian. */
  MPstr_to_num("1048576.0",    DEC, v->MPcon_vals[6]) ;  /* 2 ^ 20. */
  MPstr_to_num("0.0353", DEC, v->MPcon_vals[7]) ;  /* grams <=> ounce. */
  MPstr_to_num("0.948",  DEC, v->MPcon_vals[8]) ;  /* Kjoules <=> BTU's. */
  MPstr_to_num("0.0610", DEC, v->MPcon_vals[9]) ;  /* cms3 <=> inches3. */

  for (i = 0; i < MAXFONTS;       i++) v->fontnames[i]    = NULL ;
  for (i = 0; i < MAXITEMS;       i++) v->item_text[i][0] = NULL ;
  for (i = 0; i < TITEMS;         i++) v->bstate[i]       = B_NORMAL ;
  for (i = 0; i < CALC_COLORSIZE; i++) v->colstr[i]       = NULL ;

  for (i = 0; i < (MAXMODES-1)*MODEKEYS; i++) v->mode_bstate[i] = B_NORMAL ;

/* Nullify the mode buttons that don't exist yet. */

  for (i = 9;  i < 16; i++) v->mode_bstate[i] = B_NULL ;   /* Financial. */
  for (i = 20; i < 24; i++) v->mode_bstate[i] = B_NULL ;   /* Logical. */
  for (i = 29; i < 32; i++) v->mode_bstate[i] = B_NULL ;
  v->mode_bstate[39] = B_NULL ;                            /* Scientific. */
  v->mode_bstate[46] = B_NULL ;
  v->mode_bstate[47] = B_NULL ;

  for (i = 0 ; i < MAXITEMS; i++) v->items[i].font = SFONT ;
  v->items[(int) DISPLAYITEM].font = BFONT ;

  n = 0 ;
  for (i = 0; i < MAXREGS; i++) mpcim(&n, v->MPmvals[i]) ;

  s = (int) S_SMALL ;
  v->minheight = ((BROWS * scaleH[s]) + ((BROWS - 1) * gsizes[s]) +
                 (2 * bsizes[s])) * 4 / 3 ;
  v->minwidth  = (BCOLS * scaleW[s])  + ((BCOLS - 1) * gsizes[s]) +
                 (2 * bsizes[s]) ;

  v->histart = -1 ;     /* No portion of numeric display selected. */
  MEMSET(v->disp_state, FALSE, MAXLINE) ;
}


void
read_rcfiles()   /* Read .calctoolrc's from home and current directories. */
{
  char *home ;                  /* Pathname for users home directory. */
  char name[MAXLINE] ;          /* Full name of users .calctoolrc file. */
  char pathname[MAXPATHLEN] ;   /* Current working directory. */
  char tmp[MAXLINE] ;           /* For temporary constant string creation. */
  int n ;
  struct passwd *entry ;

  for (n = 0; n < MAXREGS; n++)
    {
      STRCPY(tmp, make_number(v->MPcon_vals[n])) ;
      SPRINTF(name, "%1d: %s [%s]", n, tmp, v->con_names[n]) ;

      STRCPY(v->con_names[n], name) ;
      STRCPY(v->fun_vals[n], "") ;    /* Initially empty function strings. */
    }

  if ((home = getenv("HOME")) == NULL)
    {
      if ((entry = getpwuid(getuid())) == NULL) return ;
      home = entry->pw_dir ;
    }
  SPRINTF(name, "%s/%s", home, RCNAME) ;
  get_rcfile(name) ;      /* Read .calctoolrc from users home directory. */

  SPRINTF(name, "%s/%s", getcwd(pathname, MAXPATHLEN+1), RCNAME) ;
  get_rcfile(name) ;      /* Read .calctoolrc file from current directory. */
}


void
read_resources()    /* Read all possible resources from the database. */
{
  int boolval, i, intval ;
  char str[MAXLINE] ;

  if (get_int_resource(R_ACCURACY, &intval))
    {
      v->accuracy = intval ;
      if (v->accuracy < 0 || v->accuracy > 9)
        {
          FPRINTF(stderr, opts[(int) O_ACCRANGE], v->progname) ;
          v->accuracy = 2 ;
        }
    }

  if (get_str_resource(R_BASE, str))
    {
      for (i = 0; i < MAXBASES; i++)
        if (EQUAL(str, Rbstr[i])) break ;

      if (i == MAXBASES)
        FPRINTF(stderr, opts[(int) O_BASE], v->progname) ;
      else
        {
          v->base = (enum base_type) i ;
        }
    }

  if (get_str_resource(R_DISPLAY, str))
    {
      for (i = 0; i < MAXDISPMODES; i++)
        if (EQUAL(str, Rdstr[i])) break ;

      if (i == MAXDISPMODES)
        FPRINTF(stderr, opts[(int) O_DISPLAY], v->progname, str) ;
      else v->dtype = (enum num_type) i ;
    }

  if (get_str_resource(R_MODE, str))
    {
      for (i = 0; i < MAXMODES; i++)
        if (EQUAL(str, Rmstr[i])) break ;

      if (i == MAXMODES)
        FPRINTF(stderr, opts[(int) O_MODE], v->progname, str) ;
      else v->modetype = (enum mode_type) i ;
    }

  if (get_str_resource(R_TRIG, str))
    {  
      for (i = 0; i < MAXTRIGMODES; i++)
        if (EQUAL(str, Rtstr[i])) break ;
       
      if (i == MAXTRIGMODES)
        FPRINTF(stderr, opts[(int) O_TRIG], v->progname, str) ;
      else v->ttype = (enum trig_type) i ;
    }

  if (get_str_resource(R_BUTFONT,  str))
    read_str(&v->fontnames[(int) NFONT], str) ;
  if (get_str_resource(R_MODEFONT, str))
    read_str(&v->fontnames[(int) SFONT], str) ;
  if (get_str_resource(R_MEMFONT,  str))
    read_str(&v->fontnames[(int) MFONT], str) ;
  if (get_str_resource(R_DISPFONT, str))
    read_str(&v->fontnames[(int) BFONT], str) ;

  check_ow_beep() ;    /* See if OpenWindows.beep is set. */

  if (get_bool_resource(R_BEEP,   &boolval)) v->beep       = boolval ;
  if (get_bool_resource(R_REGS,   &boolval)) v->rstate     = boolval ;
  if (get_bool_resource(R_THREED, &boolval)) v->is_3D      = boolval ;
  if (get_bool_resource(R_MONO,   &boolval)) v->monochrome = boolval ;
  if (get_bool_resource(R_RHAND,  &boolval)) v->righthand  = boolval ;
  if (get_bool_resource(R_TITLE,  &boolval)) v->istitle    = boolval ;

  if (get_str_resource(R_DECDIG,   str)) read_str(&v->colstr[C_DECDIG],   str) ;
  if (get_str_resource(R_HEXDIG,   str)) read_str(&v->colstr[C_HEXDIG],   str) ;
  if (get_str_resource(R_ARITHOP,  str)) read_str(&v->colstr[C_ARITHOP],  str) ;
  if (get_str_resource(R_ADJUST,   str)) read_str(&v->colstr[C_ADJUST],   str) ;
  if (get_str_resource(R_PORTION,  str)) read_str(&v->colstr[C_PORTION],  str) ;
  if (get_str_resource(R_FUNC,     str)) read_str(&v->colstr[C_FUNC],     str) ;
  if (get_str_resource(R_MAINMODE, str)) read_str(&v->colstr[C_MAINMODE], str) ;
  if (get_str_resource(R_PLOGICAL, str)) read_str(&v->colstr[C_PLOGICAL], str) ;
  if (get_str_resource(R_BLOGICAL, str)) read_str(&v->colstr[C_BLOGICAL], str) ;
  if (get_str_resource(R_FIN,      str)) read_str(&v->colstr[C_FIN],      str) ;
  if (get_str_resource(R_TRIGMODE, str)) read_str(&v->colstr[C_TRIGMODE], str) ;
  if (get_str_resource(R_TRIGCOL,  str)) read_str(&v->colstr[C_TRIGCOL],  str) ;
  if (get_str_resource(R_SCI,      str)) read_str(&v->colstr[C_SCI],      str) ;
  if (get_str_resource(R_BACK,     str)) read_str(&v->colstr[C_BACK],     str) ;
  if (get_str_resource(R_DISPCOL,  str)) read_str(&v->colstr[C_DISPCOL],  str) ;
  if (get_str_resource(R_MEMORY,   str)) read_str(&v->colstr[C_MEMORY],   str) ;
  if (get_str_resource(R_TEXT,     str)) read_str(&v->colstr[C_TEXT],     str) ;
}


void
read_str(str, value)
char **str, *value ;
{
  if (*str != NULL) (void) free(*str) ;
  if (value != NULL && strlen(value))
    {
      *str = (char *) malloc((unsigned) (strlen(value) + 1)) ;
      STRCPY(*str, value) ;
    }
  else *str = NULL ;
}


char *
set_bool(value)
int value ;
{
  return((value) ? vstrs[(int) V_TRUE] : vstrs[(int) V_FALSE]) ;
}


void
set_def_vals()
{
  v->monochrome    = 0 ;      /* Display defaults to screen type. */
  v->is_3D         = 0 ;      /* 2D look (on color screens) by default. */
  v->righthand     = 1 ;      /* "Right-handed" calculator by default. */
}


void
usage(progname)
char *progname ;
{
  FPRINTF(stderr, ustrs[(int) USAGE1], progname, PATCHLEVEL) ;
  FPRINTF(stderr, ustrs[(int) USAGE2], progname) ;
  FPRINTF(stderr, ustrs[(int) USAGE3]) ;
  exit(1) ;
}


void
write_cmdline()
{
  int argc ;
  char *argv[15], buf[MAXLINE] ;

  argc = 0 ;
  if (v->is_3D)      argv[argc++] = cmdstr[(int) CMD_3D] ;
  else               argv[argc++] = cmdstr[(int) CMD_2D] ;

  if (v->monochrome) argv[argc++] = cmdstr[(int) CMD_MONO] ;
  else               argv[argc++] = cmdstr[(int) CMD_COLOR] ;

  if (v->righthand)  argv[argc++] = cmdstr[(int) CMD_RIGHTH] ;
  else               argv[argc++] = cmdstr[(int) CMD_LEFTH] ;

  if (v->istitle)    argv[argc++] = cmdstr[(int) CMD_TITLE] ;
  else               argv[argc++] = cmdstr[(int) CMD_NOTITLE] ;

  argv[argc++] = cmdstr[(int) CMD_ACC] ;
  SPRINTF(buf, "%1d", v->accuracy) ;
  argv[argc++] = buf ;

  save_cmdline(argc, argv) ;
}


void
write_rcfile(mtype, exists, cfno, val, comment)
enum menu_type mtype ;
int exists, cfno ;
char *val, *comment ;
{
  char *home ;                  /* Pathname for users home directory. */
  char pathname[MAXPATHLEN] ;   /* Current working directory. */
  char rcname[MAXPATHLEN] ;     /* Full name of users .calctoolrc file. */
  char str[MAXLINE] ;           /* Temporary buffer. */
  char sval[3] ;                /* Used for string comparisons. */
  char tmp_filename[MAXLINE] ;  /* Used to construct temp filename. */
  int rcexists ;                /* Set to 1, if .calctoolrc file exists. */
  FILE *rcfd ;                  /* File descriptor for .calctoolrc file. */
  FILE *tmpfd ;                 /* File descriptor for new temp .calctoolrc. */
  struct passwd *entry ;        /* The user's /etc/passwd entry. */

  rcexists = 0 ;
  SPRINTF(rcname, "%s/%s", getcwd(pathname, MAXPATHLEN+1), RCNAME) ;
  if (access(rcname, F_OK) == 0) rcexists = 1 ;
  else
    { 
      if ((home = getenv("HOME")) == NULL)
        {
          if ((entry = getpwuid(getuid())) == NULL) return ;
          home = entry->pw_dir ;
        }
      SPRINTF(rcname, "%s/%s", home, RCNAME) ;
      if (access(rcname, F_OK) == 0) rcexists = 1 ;
    }
  STRCPY(tmp_filename, "/tmp/.calctoolrcXXXXXX") ;
  MKTEMP(tmp_filename) ;
  if ((tmpfd = fopen(tmp_filename, "w+")) == NULL) return ;

  if (rcexists)
    {
      rcfd = fopen(rcname, "r") ;
      SPRINTF(sval, " %1d", cfno) ;
      while (fgets(str, MAXLINE, rcfd))
        {
          if (exists)
            {
              switch (mtype)
                {
                  case M_CON : sval[0] = 'c' ;
                               if (!strncmp(str, sval, 2)) FPUTS("#", tmpfd) ;
                               sval[0] = 'C' ;
                               if (!strncmp(str, sval, 2)) FPUTS("#", tmpfd) ;
                               break ;
                  case M_FUN : sval[0] = 'f' ;
                               if (!strncmp(str, sval, 2)) FPUTS("#", tmpfd) ;
                               sval[0] = 'F' ;
                               if (!strncmp(str, sval, 2)) FPUTS("#", tmpfd) ;
                }
            }
          FPUTS(str, tmpfd) ;
        }
      FCLOSE(rcfd) ;
    }

  switch (mtype)
    {
      case M_CON : FPRINTF(tmpfd, "\nC%1d %s %s\n", cfno, val, comment) ;
                   break ;
      case M_FUN : FPRINTF(tmpfd, "\nF%1d %s %s\n", cfno, val, comment) ;
    }
  UNLINK(rcname) ;
  rcfd = fopen(rcname, "w") ;
  REWIND(tmpfd) ;
  while (fgets(str, MAXLINE, tmpfd)) FPUTS(str, rcfd) ;
  FCLOSE(rcfd) ;
  FCLOSE(tmpfd);
  UNLINK(tmp_filename) ;
}


void
write_resources()
{
  char intval[5], *value ;

  load_deskset_defs() ;

  SPRINTF(intval, "%d", v->accuracy) ;
  put_resource(R_ACCURACY, intval) ;
  put_resource(R_BASE,     Rbstr[(int) v->base]) ;
  put_resource(R_DISPLAY,  Rdstr[(int) v->dtype]) ;
  put_resource(R_MODE,     Rmstr[(int) v->modetype]) ;
  put_resource(R_TRIG,     Rtstr[(int) v->ttype]) ;

  if (v->fontnames[(int) BFONT] != NULL)
    put_resource(R_DISPFONT, v->fontnames[(int) BFONT]) ;
  if (v->fontnames[(int) MFONT] != NULL)
    put_resource(R_MEMFONT,  v->fontnames[(int) MFONT]) ;
  if (v->fontnames[(int) NFONT] != NULL)
    put_resource(R_BUTFONT,  v->fontnames[(int) NFONT]) ;
  if (v->fontnames[(int) SFONT] != NULL)
    put_resource(R_MODEFONT, v->fontnames[(int) SFONT]) ;

  put_resource(R_MONO,   set_bool(v->monochrome == TRUE)) ;
  put_resource(R_REGS,   set_bool(v->rstate == TRUE)) ;
  put_resource(R_RHAND,  set_bool(v->righthand == TRUE)) ;
  put_resource(R_THREED, set_bool(v->is_3D == TRUE)) ;
  put_resource(R_TITLE,  set_bool(v->istitle == TRUE)) ;
  put_resource(R_BEEP,   set_bool(v->beep == TRUE)) ;
  save_resources() ;
}
