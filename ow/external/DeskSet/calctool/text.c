#ifndef lint
static char sccsid[] = "@(#)text.c 1.9 94/04/28 Copyr 1987 Sun Micro";
#endif

/*  The DeskSet calculator.
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

#include <stdio.h>
#include <string.h>
#include "color.h"
#include "calctool.h"
#include "extern.h"

/*  The following are all the static strings used by the calctool program.
 *  They are initialised in init_text() to the local language equivalents.
 */

char *base_str[]  = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *calc_res[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *cmdstr[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *dtype_str[] = {
  (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *hstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;

char *lstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;

char *mess[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;

char *mode_str[]  = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *mpstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *mstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *opts[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL
} ;

char *sstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *ttype_str[] = {
  (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *ustrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL
} ;

char *vstrs[] = {
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
  (char *) NULL, (char *) NULL, (char *) NULL, (char *) NULL,
} ;


void
init_cmdline_opts()      /* Initialise command line option strings. */
{
  cmdstr[(int) CMD_2D]       = "-2" ;
  cmdstr[(int) CMD_3D]       = "-3" ;
  cmdstr[(int) CMD_ACC]      = "-a" ;
  cmdstr[(int) CMD_COLOR]    = "-c" ;
  cmdstr[(int) CMD_LEFTH]    = "-l" ;
  cmdstr[(int) CMD_MONO]     = "-m" ;
  cmdstr[(int) CMD_RIGHTH]   = "-r" ;
  cmdstr[(int) CMD_TITLE]    = "+Wn" ;
  cmdstr[(int) CMD_NOTITLE]  = "-Wn" ;
  cmdstr[(int) CMD_NAME]     = "-name" ;
  cmdstr[(int) CMD_WN]       = "Wn" ;
}


void
init_text()   /* Setup text strings depending upon language. */
{
  int i ;

  base_str[(int) BIN] = LGET("BIN") ;    /* Base values. */
  base_str[(int) OCT] = LGET("OCT") ;
  base_str[(int) DEC] = LGET("DEC") ;
  base_str[(int) HEX] = LGET("HEX") ;

/*  Some notes for translators:
 *
 *  Text in the label strings below, should be left justified. It will
 *  automatically be centre justified in the buttons, but various
 *  calculations depend on the text starting on the left.
 *
 *  The original English V3 version was written with fixed (upto) four
 *  character button labels. I've tried to extend this to allow any sized
 *  labels, however the following conditions apply:
 *
 *  - If the label won't fit in the button, then as much as possible will be
 *    shown, with the last character being '>'. If you enlarge the calculator,
 *    probably more of the label will show.
 *
 *  - If the last character of the label is a '.', and this is not the first
 *    character of the label (ie, the numeric point label), then calctool
 *    knows that this signifies that this label should have two more '.'
 *    characters appended to it (ie, "Mem." becomes "Mem...").
 *
 *  - If a calctool button has a menu item glyph associated with it, the
 *    width of this glyph is taken into consideration, and the amount of
 *    the label that can be displayed is adjusted accordingly.
 */

  i = 0 ;
  read_str(&buttons[i++].hstr, DGET("D   ")) ;      /* Row 1. */
  read_str(&buttons[i++].hstr, DGET("E   ")) ;
  read_str(&buttons[i++].hstr, DGET("F   ")) ;
  read_str(&buttons[i++].hstr, DGET("Clr ")) ;
  read_str(&buttons[i++].hstr, DGET("Int ")) ;
  read_str(&buttons[i++].hstr, DGET("Frac")) ;
  read_str(&buttons[i++].hstr, DGET("Base")) ;
  read_str(&buttons[i++].hstr, DGET("Disp")) ;

  read_str(&buttons[i++].hstr, DGET("A   ")) ;      /* Row 2. */
  read_str(&buttons[i++].hstr, DGET("B   ")) ;
  read_str(&buttons[i++].hstr, DGET("C   ")) ;
  read_str(&buttons[i++].hstr, DGET("Bsp ")) ;
  read_str(&buttons[i++].hstr, DGET("Abs ")) ;
  read_str(&buttons[i++].hstr, DGET("+/- ")) ;
  read_str(&buttons[i++].hstr, DGET("Keys")) ;
  read_str(&buttons[i++].hstr, DGET("Mode")) ;

  read_str(&buttons[i++].hstr, DGET("7   ")) ;      /* Row 3. */
  read_str(&buttons[i++].hstr, DGET("8   ")) ;
  read_str(&buttons[i++].hstr, DGET("9   ")) ;
  read_str(&buttons[i++].hstr, DGET("X   ")) ;
  read_str(&buttons[i++].hstr, DGET("1/x ")) ;
  read_str(&buttons[i++].hstr, DGET("x^2 ")) ;
  read_str(&buttons[i++].hstr, DGET("Acc ")) ;
  read_str(&buttons[i++].hstr, DGET("Mem.")) ;

  read_str(&buttons[i++].hstr, DGET("4   ")) ;      /* Row 4. */
  read_str(&buttons[i++].hstr, DGET("5   ")) ;
  read_str(&buttons[i++].hstr, DGET("6   ")) ;
  read_str(&buttons[i++].hstr, DGET("/   ")) ;
  read_str(&buttons[i++].hstr, DGET("%   ")) ;
  read_str(&buttons[i++].hstr, DGET("Sqrt")) ;
  read_str(&buttons[i++].hstr, DGET("Con ")) ;
  read_str(&buttons[i++].hstr, DGET("Fun ")) ;

  read_str(&buttons[i++].hstr, DGET("1   ")) ;      /* Row 5. */
  read_str(&buttons[i++].hstr, DGET("2   ")) ;
  read_str(&buttons[i++].hstr, DGET("3   ")) ;
  read_str(&buttons[i++].hstr, DGET("-   ")) ;
  read_str(&buttons[i++].hstr, DGET("(   ")) ;
  read_str(&buttons[i++].hstr, DGET(")   ")) ;
  read_str(&buttons[i++].hstr, DGET("Rcl ")) ;
  read_str(&buttons[i++].hstr, DGET("Sto ")) ;

  read_str(&buttons[i++].hstr, DGET("0   ")) ;      /* Row 6. */
  read_str(&buttons[i++].hstr, DGET(".   ")) ;
  read_str(&buttons[i++].hstr, DGET("=   ")) ;
  read_str(&buttons[i++].hstr, DGET("+   ")) ;
  read_str(&buttons[i++].hstr, DGET("Exp ")) ;
  read_str(&buttons[i++].hstr, DGET("Asc.")) ;
  read_str(&buttons[i++].hstr, DGET("Exch")) ;
  read_str(&buttons[i++].hstr, DGET("Quit")) ;

  i = 0 ;
  read_str(&mode_buttons[i++].hstr, DGET("Ctrm")) ;          /* Financial. */
  read_str(&mode_buttons[i++].hstr, DGET("Ddb ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Fv  ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Pmt ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Pv  ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Rate")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Sln ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Syd ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Term")) ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;

  read_str(&mode_buttons[i++].hstr, DGET(" <  ")) ;          /* Logical. */
  read_str(&mode_buttons[i++].hstr, DGET(" >  ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("&16 ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("&32 ")) ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, DGET("Or  ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("And ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Not ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Xor ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Xnor")) ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;

  read_str(&mode_buttons[i++].hstr, DGET("Trig")) ;          /* Scientific. */
  read_str(&mode_buttons[i++].hstr, DGET("Hyp ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Inv ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("e^x ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("10^x")) ;
  read_str(&mode_buttons[i++].hstr, DGET("y^x ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("x!  ")) ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, DGET("Cos ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Sin ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Tan ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Ln  ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Log ")) ;
  read_str(&mode_buttons[i++].hstr, DGET("Rand")) ;
  read_str(&mode_buttons[i++].hstr, "    ") ;
  read_str(&mode_buttons[i++].hstr, "    ") ;

  i = 0 ;
  read_str(&buttons[i++].str, LGET("D   ")) ;      /* Row 1. */
  read_str(&buttons[i++].str, LGET("E   ")) ;
  read_str(&buttons[i++].str, LGET("F   ")) ;
  read_str(&buttons[i++].str, LGET("Clr ")) ;
  read_str(&buttons[i++].str, LGET("Int ")) ;
  read_str(&buttons[i++].str, LGET("Frac")) ;
  read_str(&buttons[i++].str, LGET("Base")) ;
  read_str(&buttons[i++].str, LGET("Disp")) ;

  read_str(&buttons[i++].str, LGET("A   ")) ;      /* Row 2. */
  read_str(&buttons[i++].str, LGET("B   ")) ;
  read_str(&buttons[i++].str, LGET("C   ")) ;
  read_str(&buttons[i++].str, LGET("Bsp ")) ;
  read_str(&buttons[i++].str, LGET("Abs ")) ;
  read_str(&buttons[i++].str, LGET("+/- ")) ;
  read_str(&buttons[i++].str, LGET("Keys")) ;
  read_str(&buttons[i++].str, LGET("Mode")) ;

  read_str(&buttons[i++].str, LGET("7   ")) ;      /* Row 3. */
  read_str(&buttons[i++].str, LGET("8   ")) ;
  read_str(&buttons[i++].str, LGET("9   ")) ;
  read_str(&buttons[i++].str, LGET("X   ")) ;
  read_str(&buttons[i++].str, LGET("1/x ")) ;
  read_str(&buttons[i++].str, LGET("x^2 ")) ;
  read_str(&buttons[i++].str, LGET("Acc ")) ;
  read_str(&buttons[i++].str, LGET("Mem.")) ;

  read_str(&buttons[i++].str, LGET("4   ")) ;      /* Row 4. */
  read_str(&buttons[i++].str, LGET("5   ")) ;
  read_str(&buttons[i++].str, LGET("6   ")) ;
  read_str(&buttons[i++].str, LGET("/   ")) ;
  read_str(&buttons[i++].str, LGET("%   ")) ;
  read_str(&buttons[i++].str, LGET("Sqrt")) ;
  read_str(&buttons[i++].str, LGET("Con ")) ;
  read_str(&buttons[i++].str, LGET("Fun ")) ;

  read_str(&buttons[i++].str, LGET("1   ")) ;      /* Row 5. */
  read_str(&buttons[i++].str, LGET("2   ")) ;
  read_str(&buttons[i++].str, LGET("3   ")) ;
  read_str(&buttons[i++].str, LGET("-   ")) ;
  read_str(&buttons[i++].str, LGET("(   ")) ;
  read_str(&buttons[i++].str, LGET(")   ")) ;
  read_str(&buttons[i++].str, LGET("Rcl ")) ;
  read_str(&buttons[i++].str, LGET("Sto ")) ;

  read_str(&buttons[i++].str, LGET("0   ")) ;      /* Row 6. */
  read_str(&buttons[i++].str, LGET(".   ")) ;
  read_str(&buttons[i++].str, LGET("=   ")) ;
  read_str(&buttons[i++].str, LGET("+   ")) ;
  read_str(&buttons[i++].str, LGET("Exp ")) ;
  read_str(&buttons[i++].str, LGET("Asc.")) ;
  read_str(&buttons[i++].str, LGET("Exch")) ;
  read_str(&buttons[i++].str, LGET("Quit")) ;

/* Keyboard equivalents for calctool main window buttons. */

/*  STRING_EXTRACTION SUNW_DESKSET_CALCTOOL_LABEL
 *
 *  The keyboard equivalents of the calctool numeric digits (including
 *  hexidecimal digits) should not be translated.
 */

  i = 0 ;
  get_key_val(&buttons[i++].value, LGET("d")) ;         /* Row 1. */
  get_key_val(&buttons[i++].value, LGET("e")) ;
  get_key_val(&buttons[i++].value, LGET("f")) ;
  get_key_val(&buttons[i++].value, LGET("\177")) ;      /* del */
  get_key_val(&buttons[i++].value, LGET("\011")) ;      /* CTL('i') */
  get_key_val(&buttons[i++].value, LGET("\006")) ;      /* CTL('f') */
  get_key_val(&buttons[i++].value, LGET("B")) ;
  get_key_val(&buttons[i++].value, LGET("D")) ;

  get_key_val(&buttons[i++].value, LGET("a")) ;         /* Row 2. */
  get_key_val(&buttons[i++].value, LGET("b")) ;
  get_key_val(&buttons[i++].value, LGET("c")) ;
  get_key_val(&buttons[i++].value, LGET("\010")) ;      /* CTL('h') */
  get_key_val(&buttons[i++].value, LGET("\025")) ;      /* CTL('u') */
  get_key_val(&buttons[i++].value, LGET("C")) ;
  get_key_val(&buttons[i++].value, LGET("k")) ;
  get_key_val(&buttons[i++].value, LGET("M")) ;

  get_key_val(&buttons[i++].value, LGET("7")) ;         /* Row 3. */
  get_key_val(&buttons[i++].value, LGET("8")) ;
  get_key_val(&buttons[i++].value, LGET("9")) ;
  get_key_val(&buttons[i++].value, LGET("x")) ;
  get_key_val(&buttons[i++].value, LGET("r")) ;
  get_key_val(&buttons[i++].value, LGET("@")) ;
  get_key_val(&buttons[i++].value, LGET("A")) ;
  get_key_val(&buttons[i++].value, LGET("m")) ;

  get_key_val(&buttons[i++].value, LGET("4")) ;         /* Row 4. */
  get_key_val(&buttons[i++].value, LGET("5")) ;
  get_key_val(&buttons[i++].value, LGET("6")) ;
  get_key_val(&buttons[i++].value, LGET("/")) ;
  get_key_val(&buttons[i++].value, LGET("%")) ;
  get_key_val(&buttons[i++].value, LGET("s")) ;
  get_key_val(&buttons[i++].value, LGET("#")) ;
  get_key_val(&buttons[i++].value, LGET("F")) ;

  get_key_val(&buttons[i++].value, LGET("1")) ;         /* Row 5. */
  get_key_val(&buttons[i++].value, LGET("2")) ;
  get_key_val(&buttons[i++].value, LGET("3")) ;
  get_key_val(&buttons[i++].value, LGET("-")) ;
  get_key_val(&buttons[i++].value, LGET("(")) ;
  get_key_val(&buttons[i++].value, LGET(")")) ;
  get_key_val(&buttons[i++].value, LGET("R")) ;
  get_key_val(&buttons[i++].value, LGET("S")) ;

  get_key_val(&buttons[i++].value, LGET("0")) ;         /* Row 6. */
  get_key_val(&buttons[i++].value, LGET(".")) ;
  get_key_val(&buttons[i++].value, LGET("=")) ;
  get_key_val(&buttons[i++].value, LGET("+")) ;
  get_key_val(&buttons[i++].value, LGET("E")) ;
  get_key_val(&buttons[i++].value, LGET("\001")) ;      /* CTL('a') */
  get_key_val(&buttons[i++].value, LGET("X")) ;
  get_key_val(&buttons[i++].value, LGET("q")) ;

  calc_res[(int) R_ACCURACY] = DGET("accuracy") ;
  calc_res[(int) R_BASE]     = DGET("base") ;
  calc_res[(int) R_DISPLAY]  = DGET("display") ;
  calc_res[(int) R_MODE]     = DGET("mode") ;
  calc_res[(int) R_MONO]     = DGET("mono") ;
  calc_res[(int) R_REGS]     = DGET("showRegisters") ;
  calc_res[(int) R_RHAND]    = DGET("rightHanded") ;
  calc_res[(int) R_THREED]   = DGET("3dLook") ;
  calc_res[(int) R_TITLE]    = DGET("hasTitle") ;
  calc_res[(int) R_TRIG]     = DGET("trigType") ;
  calc_res[(int) R_DECDIG]   = DGET("decDigitColor") ;
  calc_res[(int) R_HEXDIG]   = DGET("hexDigitColor") ;
  calc_res[(int) R_ARITHOP]  = DGET("arithOpColor") ;
  calc_res[(int) R_ADJUST]   = DGET("adjustColor") ;
  calc_res[(int) R_PORTION]  = DGET("portionColor") ;
  calc_res[(int) R_FUNC]     = DGET("functionColor") ;
  calc_res[(int) R_MAINMODE] = DGET("mainModeColor") ;
  calc_res[(int) R_PLOGICAL] = DGET("portionLogicalColor") ;
  calc_res[(int) R_BLOGICAL] = DGET("bitLogicalColor") ;
  calc_res[(int) R_FIN]      = DGET("finColor") ;
  calc_res[(int) R_TRIGMODE] = DGET("trigModeColor") ;
  calc_res[(int) R_TRIGCOL]  = DGET("trigColor") ;
  calc_res[(int) R_SCI]      = DGET("sciColor") ;
  calc_res[(int) R_BACK]     = DGET("backgroundColor") ;
  calc_res[(int) R_DISPCOL]  = DGET("displayColor") ;
  calc_res[(int) R_MEMORY]   = DGET("memRegisterColor") ;
  calc_res[(int) R_TEXT]     = DGET("textColor") ;
  calc_res[(int) R_BUTFONT]  = DGET("buttonFont") ;
  calc_res[(int) R_MODEFONT] = DGET("modeFont") ;
  calc_res[(int) R_MEMFONT]  = DGET("memoryFont") ;
  calc_res[(int) R_DISPFONT] = DGET("displayFont") ;
  calc_res[(int) R_BEEP]     = DGET("beep") ;

  cmenus[(int) M_CON].title = LGET("Constants") ;       /* CON */
  cmenus[(int) M_FUN].title = LGET("Functions") ;       /* FUN */

  STRCPY(v->con_names[0], LGET("kilometres per hour <=> miles per hour.")) ;
  STRCPY(v->con_names[1], LGET("square root of 2.")) ;
  STRCPY(v->con_names[2], LGET("e.")) ;
  STRCPY(v->con_names[3], LGET("pi.")) ;
  STRCPY(v->con_names[4], LGET("centimetres <=> inch.")) ;
  STRCPY(v->con_names[5], LGET("degrees in a radian.")) ;
  STRCPY(v->con_names[6], LGET("2 ^ 20.")) ;
  STRCPY(v->con_names[7], LGET("grams <=> ounce.")) ;
  STRCPY(v->con_names[8], LGET("kilojoules <=> British thermal units.")) ;
  STRCPY(v->con_names[9], LGET("cubic cms <=> cubic inches.")) ;

  dtype_str[(int) ENG] = LGET("ENG") ;                  /* Display mode. */
  dtype_str[(int) FIX] = LGET("FIX") ;
  dtype_str[(int) SCI] = LGET("SCI") ;

/*  XView help strings.
 *
 *  NOTE: The first thirteen help items correspond to the thirteen menu
 *        items. They must be kept in the same order.
 */

  hstrs[(int) H_ACC]        = DGET("calctool:Acc ") ;
  hstrs[(int) H_BASE]       = DGET("calctool:Base") ;
  hstrs[(int) H_CON]        = DGET("calctool:Con ") ;
  hstrs[(int) H_EXCH]       = DGET("calctool:Exch") ;
  hstrs[(int) H_FUN]        = DGET("calctool:Fun ") ;
  hstrs[(int) H_LSHF]       = DGET("calctool: <  ") ;
  hstrs[(int) H_MODE]       = DGET("calctool:Mode") ;
  hstrs[(int) H_NUM]        = DGET("calctool:Disp") ;
  hstrs[(int) H_RCL]        = DGET("calctool:Rcl ") ;
  hstrs[(int) H_RSHF]       = DGET("calctool: >  ") ;
  hstrs[(int) H_STO]        = DGET("calctool:Sto ") ;
  hstrs[(int) H_TRIG]       = DGET("calctool:Trig") ;
  hstrs[(int) H_PROPS]      = DGET("calctool:Props") ;

  hstrs[(int) H_ABUT]       = DGET("calctool:AscButton") ;
  hstrs[(int) H_AFRAME]     = DGET("calctool:AscFrame") ;
  hstrs[(int) H_APANEL]     = DGET("calctool:AscPanel") ;
  hstrs[(int) H_APPEARANCE] = DGET("calctool:AppearanceChoice") ;
  hstrs[(int) H_APPLY]      = DGET("calctool:ApplyButton") ;
  hstrs[(int) H_ATEXT]      = DGET("calctool:AscText") ;
  hstrs[(int) H_CFCBUT]     = DGET("calctool:ConFunConButton") ;
  hstrs[(int) H_CFDESC]     = DGET("calctool:ConFunDescription") ;
  hstrs[(int) H_CFFBUT]     = DGET("calctool:ConFunFunButton") ;
  hstrs[(int) H_CFFRAME]    = DGET("calctool:ConFunFrame") ;
  hstrs[(int) H_CFNO]       = DGET("calctool:ConFunNumber") ;
  hstrs[(int) H_CFPANEL]    = DGET("calctool:ConFunPanel") ;
  hstrs[(int) H_CFVAL]      = DGET("calctool:ConFunValue") ;
  hstrs[(int) H_DEF]        = DGET("calctool:DefaultButton") ;
  hstrs[(int) H_DISPLAY]    = DGET("calctool:DisplayChoice") ;
  hstrs[(int) H_MCANVAS]    = DGET("calctool:MemCanvas") ;
  hstrs[(int) H_MFRAME]     = DGET("calctool:MemFrame") ;
  hstrs[(int) H_PFRAME]     = DGET("calctool:PropertyFrame") ;
  hstrs[(int) H_PPANEL]     = DGET("calctool:PropertyPanel") ;
  hstrs[(int) H_RESET]      = DGET("calctool:ResetButton") ;
  hstrs[(int) H_STYLE]      = DGET("calctool:StyleChoice") ;

/* Labels for various XView items. */
  lstrs[(int) L_CONNO]   = LGET("Constant no:") ;
  lstrs[(int) L_FUNNO]   = LGET("Function no:") ;
  lstrs[(int) L_LCALC]   = LGET("calculator") ;
  lstrs[(int) L_NEWCON]  = LGET("New Constant") ;
  lstrs[(int) L_NEWFUN]  = LGET("New Function") ;
  lstrs[(int) L_UCALC]   = LGET("Calculator") ;

/* Message strings. */
  mess[(int) MESS_COLOR] = MGET("%s: cannot allocate colors. ") ;
  mess[(int) MESS_FONT]  = MGET("%s: couldn't get the default font.") ;
  mess[(int) MESS_MONO]  = LGET("Starting monochrome version.\n") ;
  mess[(int) MESS_PARAM] = MGET("%s: %s as next argument.\n") ;
  mess[(int) MESS_ICON]  = MGET("%s: cannot read icon filename (%s)\n") ;
  mess[(int) MESS_CON]   = MGET("%s %1d already exists.") ;

/* Mode titles for the popup panel. */
  mstrs[(int) FINANCIAL]  = LGET("Financial Mode.") ;
  mstrs[(int) LOGICAL]    = LGET("Logical Mode.") ;
  mstrs[(int) SCIENTIFIC] = LGET("Scientific Mode.") ;

  for (i = 0; i < MAXENTRIES; i++) menu_entries[i] = NULL ;

/* Keyboard equivalents for the calctool menu entries. */

          i = 0 ;
/* 00 */  get_key_val(&menu_entries[i++], LGET("0")) ;  /* ACC */
/* 01 */  get_key_val(&menu_entries[i++], LGET("1")) ;
/* 02 */  get_key_val(&menu_entries[i++], LGET("2")) ;
/* 03 */  get_key_val(&menu_entries[i++], LGET("3")) ;
/* 04 */  get_key_val(&menu_entries[i++], LGET("4")) ;
/* 05 */  get_key_val(&menu_entries[i++], LGET("5")) ;
/* 06 */  get_key_val(&menu_entries[i++], LGET("6")) ;
/* 07 */  get_key_val(&menu_entries[i++], LGET("7")) ;
/* 08 */  get_key_val(&menu_entries[i++], LGET("8")) ;
/* 09 */  get_key_val(&menu_entries[i++], LGET("9")) ;
/* 10 */  get_key_val(&menu_entries[i++], LGET("0")) ;  /* EXCH, RCL, STO */
/* 11 */  get_key_val(&menu_entries[i++], LGET("1")) ;
/* 12 */  get_key_val(&menu_entries[i++], LGET("2")) ;
/* 13 */  get_key_val(&menu_entries[i++], LGET("3")) ;
/* 14 */  get_key_val(&menu_entries[i++], LGET("4")) ;
/* 15 */  get_key_val(&menu_entries[i++], LGET("5")) ;
/* 16 */  get_key_val(&menu_entries[i++], LGET("6")) ;
/* 17 */  get_key_val(&menu_entries[i++], LGET("7")) ;
/* 18 */  get_key_val(&menu_entries[i++], LGET("8")) ;
/* 19 */  get_key_val(&menu_entries[i++], LGET("9")) ;
/* 20 */  get_key_val(&menu_entries[i++], LGET("1")) ;  /* LSHF, RSHF */
/* 21 */  get_key_val(&menu_entries[i++], LGET("2")) ;
/* 22 */  get_key_val(&menu_entries[i++], LGET("3")) ;
/* 23 */  get_key_val(&menu_entries[i++], LGET("4")) ;
/* 24 */  get_key_val(&menu_entries[i++], LGET("5")) ;
/* 25 */  get_key_val(&menu_entries[i++], LGET("6")) ;
/* 26 */  get_key_val(&menu_entries[i++], LGET("7")) ;
/* 27 */  get_key_val(&menu_entries[i++], LGET("8")) ;
/* 28 */  get_key_val(&menu_entries[i++], LGET("9")) ;
/* 29 */  get_key_val(&menu_entries[i++], LGET("a")) ;
/* 30 */  get_key_val(&menu_entries[i++], LGET("b")) ;
/* 31 */  get_key_val(&menu_entries[i++], LGET("c")) ;
/* 32 */  get_key_val(&menu_entries[i++], LGET("d")) ;
/* 33 */  get_key_val(&menu_entries[i++], LGET("e")) ;
/* 34 */  get_key_val(&menu_entries[i++], LGET("f")) ;
/* 35 */  get_key_val(&menu_entries[i++], LGET("b")) ;  /* BASE */
/* 36 */  get_key_val(&menu_entries[i++], LGET("o")) ;
/* 37 */  get_key_val(&menu_entries[i++], LGET("d")) ;
/* 38 */  get_key_val(&menu_entries[i++], LGET("h")) ;
/* 39 */  get_key_val(&menu_entries[i++], LGET("e")) ;  /* Display type. */
/* 40 */  get_key_val(&menu_entries[i++], LGET("f")) ;
/* 41 */  get_key_val(&menu_entries[i++], LGET("s")) ;
/* 42 */  get_key_val(&menu_entries[i++], LGET("d")) ;  /* Trig. type. */
/* 43 */  get_key_val(&menu_entries[i++], LGET("g")) ;
/* 44 */  get_key_val(&menu_entries[i++], LGET("r")) ;
/* 45 */  get_key_val(&menu_entries[i++], LGET("b")) ;  /* MODE */
/* 46 */  get_key_val(&menu_entries[i++], LGET("f")) ;
/* 47 */  get_key_val(&menu_entries[i++], LGET("l")) ;
/* 48 */  get_key_val(&menu_entries[i++], LGET("s")) ;
/* 49 */  get_key_val(&menu_entries[i++], LGET("\020")) ;

  i = 0 ;
  read_str(&mode_buttons[i++].str, LGET("Ctrm")) ;          /* Financial. */
  read_str(&mode_buttons[i++].str, LGET("Ddb ")) ;
  read_str(&mode_buttons[i++].str, LGET("Fv  ")) ;
  read_str(&mode_buttons[i++].str, LGET("Pmt ")) ;
  read_str(&mode_buttons[i++].str, LGET("Pv  ")) ;
  read_str(&mode_buttons[i++].str, LGET("Rate")) ;
  read_str(&mode_buttons[i++].str, LGET("Sln ")) ;
  read_str(&mode_buttons[i++].str, LGET("Syd ")) ;
  read_str(&mode_buttons[i++].str, LGET("Term")) ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;

  read_str(&mode_buttons[i++].str, LGET(" <  ")) ;          /* Logical. */
  read_str(&mode_buttons[i++].str, LGET(" >  ")) ;
  read_str(&mode_buttons[i++].str, LGET("&16 ")) ;
  read_str(&mode_buttons[i++].str, LGET("&32 ")) ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, LGET("Or  ")) ;
  read_str(&mode_buttons[i++].str, LGET("And ")) ;
  read_str(&mode_buttons[i++].str, LGET("Not ")) ;
  read_str(&mode_buttons[i++].str, LGET("Xor ")) ;
  read_str(&mode_buttons[i++].str, LGET("Xnor")) ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;

  read_str(&mode_buttons[i++].str, LGET("Trig")) ;          /* Scientific. */
  read_str(&mode_buttons[i++].str, LGET("Hyp ")) ;
  read_str(&mode_buttons[i++].str, LGET("Inv ")) ;
  read_str(&mode_buttons[i++].str, LGET("e^x ")) ;
  read_str(&mode_buttons[i++].str, LGET("10^x")) ;
  read_str(&mode_buttons[i++].str, LGET("y^x ")) ;
  read_str(&mode_buttons[i++].str, LGET("x!  ")) ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, LGET("Cos ")) ;
  read_str(&mode_buttons[i++].str, LGET("Sin ")) ;
  read_str(&mode_buttons[i++].str, LGET("Tan ")) ;
  read_str(&mode_buttons[i++].str, LGET("Ln  ")) ;
  read_str(&mode_buttons[i++].str, LGET("Log ")) ;
  read_str(&mode_buttons[i++].str, LGET("Rand")) ;
  read_str(&mode_buttons[i++].str, "    ") ;
  read_str(&mode_buttons[i++].str, "    ") ;

/* Keyboard equivalents for the three calctool mode window buttons. */

  i = 0 ;
                                                            /* Financial */
  get_key_val(&mode_buttons[i++].value, LGET("\024")) ;     /* CTL('t') */
  get_key_val(&mode_buttons[i++].value, LGET("\004")) ;     /* CTL('d') */
  get_key_val(&mode_buttons[i++].value, LGET("v")) ;
  get_key_val(&mode_buttons[i++].value, LGET("P")) ;
  get_key_val(&mode_buttons[i++].value, LGET("p")) ;
  get_key_val(&mode_buttons[i++].value, LGET("\022")) ;     /* CTL('r') */
  get_key_val(&mode_buttons[i++].value, LGET("\023")) ;     /* CTL('s') */
  get_key_val(&mode_buttons[i++].value, LGET("\031")) ;     /* CTL('y') */
  get_key_val(&mode_buttons[i++].value, LGET("T")) ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;

  get_key_val(&mode_buttons[i++].value, LGET("<")) ;        /* Logical. */
  get_key_val(&mode_buttons[i++].value, LGET(">")) ;
  get_key_val(&mode_buttons[i++].value, LGET("[")) ;
  get_key_val(&mode_buttons[i++].value, LGET("]")) ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, LGET("|")) ;
  get_key_val(&mode_buttons[i++].value, LGET("&")) ;
  get_key_val(&mode_buttons[i++].value, LGET("~")) ;
  get_key_val(&mode_buttons[i++].value, LGET("^")) ;
  get_key_val(&mode_buttons[i++].value, LGET("n")) ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;

  get_key_val(&mode_buttons[i++].value, LGET("T")) ;        /* Scientific. */
  get_key_val(&mode_buttons[i++].value, LGET("h")) ;
  get_key_val(&mode_buttons[i++].value, LGET("i")) ;
  get_key_val(&mode_buttons[i++].value, LGET("{")) ;
  get_key_val(&mode_buttons[i++].value, LGET("}")) ;
  get_key_val(&mode_buttons[i++].value, LGET("y")) ;
  get_key_val(&mode_buttons[i++].value, LGET("!")) ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, LGET("\003")) ;     /* CTL('c') */
  get_key_val(&mode_buttons[i++].value, LGET("\023")) ;     /* CTL('s') */
  get_key_val(&mode_buttons[i++].value, LGET("\024")) ;     /* CTL('t') */
  get_key_val(&mode_buttons[i++].value, LGET("N")) ;
  get_key_val(&mode_buttons[i++].value, LGET("G")) ;
  get_key_val(&mode_buttons[i++].value, LGET("?")) ;
  get_key_val(&mode_buttons[i++].value, " ") ;
  get_key_val(&mode_buttons[i++].value, " ") ;

  mode_str[(int) BASIC]      = LGET("BASIC") ;        /* Mode values. */
  mode_str[(int) FINANCIAL]  = LGET("FINANCIAL") ;
  mode_str[(int) LOGICAL]    = LGET("LOGICAL") ;
  mode_str[(int) SCIENTIFIC] = LGET("SCIENTIFIC") ;

/* MP errors (visible with the -E command line option. */

  mpstrs[(int) MP_ADD2A] = LGET("*** SIGN NOT 0, +1 OR -1 IN MPADD2 CALL.\n") ;
  mpstrs[(int) MP_ADD2B] = LGET("POSSIBLE OVERWRITING PROBLEM ***\n") ;
  mpstrs[(int) MP_PART1] = LGET("*** N .LE. 1 IN CALL TO MPART1 ***\n") ;
  mpstrs[(int) MP_ASIN]  = LGET("*** ABS(X) .GT. 1 IN CALL TO MPASIN ***\n") ;
  mpstrs[(int) MP_ATAN]  = LGET("*** ERROR OCCURRED IN MPATAN, RESULT INCORRECT ***\n") ;
  mpstrs[(int) MP_CHKC]  = MGET("*** B = %d ILLEGAL IN CALL TO MPCHK.\n") ;
  mpstrs[(int) MP_CHKD]  = LGET("PERHAPS NOT SET BEFORE CALL TO AN MP ROUTINE ***\n") ;
  mpstrs[(int) MP_CHKE]  = MGET("*** T = %d ILLEGAL IN CALL TO MPCHK.\n") ;
  mpstrs[(int) MP_CHKF]  = LGET("PERHAPS NOT SET BEFORE CALL TO AN MP ROUTINE ***\n") ;
  mpstrs[(int) MP_CHKG]  = LGET("*** M .LE. T IN CALL TO MPCHK.\n") ;
  mpstrs[(int) MP_CHKH]  = LGET("PERHAPS NOT SET BEFORE CALL TO AN MP ROUTINE ***\n") ;
  mpstrs[(int) MP_CHKI]  = LGET("*** B TOO LARGE IN CALL TO MPCHK ***\n") ;
  mpstrs[(int) MP_CHKJ]  = LGET("*** MXR TOO SMALL OR NOT SET TO DIM(R) BEFORE CALL ") ;
  mpstrs[(int) MP_CHKL]  = LGET("TO AN MP ROUTINE ***\n") ;
  mpstrs[(int) MP_CHKM]  = MGET("*** MXR SHOULD BE AT LEAST %d*T + %d = %d  ***\n") ;
  mpstrs[(int) MP_CHKN]  = MGET("*** ACTUALLY MXR = %d, AND T = %d  ***\n") ;
  mpstrs[(int) MP_CMD]   = LGET("*** FLOATING-POINT OVER/UNDER-FLOW IN MPCMD ***\n") ;
  mpstrs[(int) MP_CMR]   = LGET("*** FLOATING-POINT OVER/UNDER-FLOW IN MPCMR ***\n") ;
  mpstrs[(int) MP_CQM]   = LGET("*** J = 0 IN CALL TO MPCQM ***\n") ;
  mpstrs[(int) MP_DIVA]  = LGET("*** ATTEMPTED DIVISION BY ZERO IN CALL TO MPDIV ***\n") ;
  mpstrs[(int) MP_DIVB]  = LGET("*** OVERFLOW OCCURRED IN MPDIV ***\n") ;
  mpstrs[(int) MP_DIVIA] = LGET("*** ATTEMPTED DIVISION BY ZERO IN CALL TO MPDIVI ***\n") ;
  mpstrs[(int) MP_DIVIB] = LGET("*** INTEGER OVERFLOW IN MPDIVI, B TOO LARGE ***\n") ;
  mpstrs[(int) MP_EXPA]  = LGET("*** OVERFLOW IN SUBROUTINE MPEXP ***\n") ;
  mpstrs[(int) MP_EXPB]  = LGET("*** ERROR OCCURRED IN MPEXP, RESULT INCORRECT ***\n") ;
  mpstrs[(int) MP_EXP1]  = LGET("*** ABS(X) NOT LESS THAN 1 IN CALL TO MPEXP1 ***\n") ;
  mpstrs[(int) MP_LNA]   = LGET("*** X NONPOSITIVE IN CALL TO MPLN ***\n") ;
  mpstrs[(int) MP_LNB]   = LGET("*** ERROR IN MPLN, ITERATION NOT CONVERGING ***\n") ;
  mpstrs[(int) MP_LNSA]  = LGET("*** ABS(X) .GE. 1/B IN CALL TO MPLNS ***\n") ;
  mpstrs[(int) MP_LNSB]  = LGET("*** ERROR OCCURRED IN MPLNS.\n") ;
  mpstrs[(int) MP_LNSC]  = LGET("NEWTON ITERATION NOT CONVERGING PROPERLY ***\n") ;
  mpstrs[(int) MP_MULA]  = LGET("*** INTEGER OVERFLOW IN MPMUL, B TOO LARGE ***\n") ;
  mpstrs[(int) MP_MULB]  = LGET("*** ILLEGAL BASE B DIGIT IN CALL TO MPMUL.\n") ;
  mpstrs[(int) MP_MULC]  = LGET("POSSIBLE OVERWRITING PROBLEM ***\n") ;
  mpstrs[(int) MP_MUL2A] = LGET("*** OVERFLOW OCCURRED IN MPMUL2 ***\n") ;
  mpstrs[(int) MP_MUL2B] = LGET("*** INTEGER OVERFLOW IN MPMUL2, B TOO LARGE ***\n") ;
  mpstrs[(int) MP_MULQ]  = LGET("*** ATTEMPTED DIVISION BY ZERO IN MPMULQ ***\n") ;
  mpstrs[(int) MP_NZRA]  = LGET("*** SIGN NOT 0, +1 OR -1 IN CALL TO MPNZR.\n") ;
  mpstrs[(int) MP_NZRB]  = LGET("POSSIBLE OVERWRITING PROBLEM ***\n") ;
  mpstrs[(int) MP_NZRC]  = LGET("*** OVERFLOW OCCURRED IN MPNZR ***\n") ;
  mpstrs[(int) MP_OVFL]  = LGET("*** CALL TO MPOVFL, MP OVERFLOW OCCURRED ***\n") ;
  mpstrs[(int) MP_PI]    = LGET("*** ERROR OCCURRED IN MPPI, RESULT INCORRECT ***\n") ;
  mpstrs[(int) MP_PWRA]  = LGET("*** ATTEMPT TO RAISE ZERO TO NEGATIVE POWER IN\n") ;
  mpstrs[(int) MP_PWRB]  = LGET("CALL TO SUBROUTINE MPPWR ***\n") ;
  mpstrs[(int) MP_PWR2A] = LGET("*** X NEGATIVE IN CALL TO MPPWR2 ***\n") ;
  mpstrs[(int) MP_PWR2B] = LGET("*** X ZERO AND Y NONPOSITIVE IN CALL TO MPPWR2 ***\n") ;
  mpstrs[(int) MP_RECA]  = LGET("*** ATTEMPTED DIVISION BY ZERO IN CALL TO MPREC ***\n") ;
  mpstrs[(int) MP_RECB]  = LGET("*** ERROR OCCURRED IN MPREC, NEWTON ITERATION\n") ;
  mpstrs[(int) MP_RECC]  = LGET("NOT CONVERGING PROPERLY ***\n") ;
  mpstrs[(int) MP_RECD]  = LGET("*** OVERFLOW OCCURRED IN MPREC ***\n") ;
  mpstrs[(int) MP_ROOTA] = LGET("*** N = 0 IN CALL TO MPROOT ***\n") ;
  mpstrs[(int) MP_ROOTB] = LGET("*** ABS(N) TOO LARGE IN CALL TO MPROOT ***\n") ;
  mpstrs[(int) MP_ROOTC] = LGET("*** X = 0 AND N NEGATIVE IN CALL TO MPROOT ***\n") ;
  mpstrs[(int) MP_ROOTD] = LGET("*** X NEGATIVE AND N EVEN IN CALL TO MPROOT ***\n") ;
  mpstrs[(int) MP_ROOTE] = LGET("*** ERROR OCCURRED IN MPROOT, NEWTON ITERATION\n") ;
  mpstrs[(int) MP_ROOTF] = LGET("NOT CONVERGING PROPERLY ***\n") ;
  mpstrs[(int) MP_SETB]  = LGET("*** IDECPL .LE. 0 IN CALL TO MPSET ***\n") ;
  mpstrs[(int) MP_SETC]  = LGET("ITMAX2 TOO SMALL IN CALL TO MPSET ***\n") ;
  mpstrs[(int) MP_SETD]  = LGET("*** INCREASE ITMAX2 AND DIMENSIONS OF MP ARRAYS \n") ;
  mpstrs[(int) MP_SETE]  = MGET("TO AT LEAST %d ***\n") ;
  mpstrs[(int) MP_SIN]   = LGET("*** ERROR OCCURRED IN MPSIN, RESULT INCORRECT ***\n") ;
  mpstrs[(int) MP_SIN1]  = LGET("*** ABS(X) .GT. 1 IN CALL TO MPSIN1 ***\n") ;
  mpstrs[(int) MP_SQRT]  = LGET("*** X NEGATIVE IN CALL TO SUBROUTINE MPSQRT ***\n") ;
  mpstrs[(int) MP_TAN]   = LGET("*** TAN(X) TOO LARGE IN CALL TO MPTAN ***\n") ;

/* Command line options. */

  opts[(int) O_ACCVAL]   = LGET("-a needs accuracy value") ;
  opts[(int) O_ACCRANGE] = MGET("%s: accuracy should be in the range 0-9\n");
  opts[(int) O_BASE]     = MGET("%s: base should be 2, 8, 10 or 16\n") ;
  opts[(int) O_DISPLAY]  = MGET("%s: invalid display mode [%s]\n") ;
  opts[(int) O_MODE]     = MGET("%s: invalid mode [%s]\n") ;
  opts[(int) O_TRIG]     = MGET("%s: invalid trig. mode [%s]\n") ;

  sstrs[(int) S_SMALL]      = LGET("small") ;         /* Scales. */
  sstrs[(int) S_MEDIUM]     = LGET("medium") ;
  sstrs[(int) S_LARGE]      = LGET("large") ;
  sstrs[(int) S_EXTRALARGE] = LGET("extra_large") ;

  ttype_str[(int) DEG]      = LGET("DEG") ;           /* Trig. type values. */
  ttype_str[(int) GRAD]     = LGET("GRAD") ;
  ttype_str[(int) RAD]      = LGET("RAD") ;

/* Usage message. */

  ustrs[(int) USAGE1] = MGET("%s version 3.2.%1d\n\n") ;
  ustrs[(int) USAGE2] = MGET("Usage: %s: [-2] [-3] [-a accuracy] [-c] [-l] [-m]\n") ;
  ustrs[(int) USAGE3] = LGET("\t\t [-name app-name] [-r] [-?] [-v] [-?] [-Wn] [+Wn]\n") ;

/* Various strings. */

  vstrs[(int) V_ASC]      = LGET("ASC") ;
  vstrs[(int) V_BSP]      = LGET("bsp") ;
  vstrs[(int) V_CALCTOOL] = LGET("calctool") ;
  vstrs[(int) V_CANCEL]   = LGET("Cancel") ;
  /* Do not translate this, use in spot help */
  vstrs[(int) V_CANVAS]   = DGET("canvas") ;
  vstrs[(int) V_CONFIRM]  = LGET("Confirm") ;
  vstrs[(int) V_CONTINUE] = LGET("Continue") ;
  vstrs[(int) V_CONWNAME] = LGET("Enter Constant...") ;
  vstrs[(int) V_DEL]      = LGET("del") ;
  /* Do not translate this, use in spot help */
  vstrs[(int) V_DISP]     = DGET("DISP") ;
  vstrs[(int) V_ERROR]    = LGET("Error") ;
  /* Do not translate this because this is resource file value */
  vstrs[(int) V_FALSE]    = DGET("false") ;
  vstrs[(int) V_FUNWNAME] = LGET("Enter Function...") ;
  vstrs[(int) V_CLR]      = LGET("CLR") ;
  vstrs[(int) V_HYP]      = LGET("HYP") ;
  vstrs[(int) V_INV]      = LGET("INV") ;
  vstrs[(int) V_INVCON]   = LGET("Invalid constant value") ;
  vstrs[(int) V_INVALID]  = LGET("Invalid %s number.") ;
  vstrs[(int) V_LCON]     = LGET("constant") ;
  vstrs[(int) V_LFUN]     = LGET("function") ;
  vstrs[(int) V_NOCHANGE] = LGET("Constant not changed.") ;
  vstrs[(int) V_NUMSTACK] = LGET("Numeric stack error") ;
  vstrs[(int) V_OPSTACK]  = LGET("Operand stack error") ;
  vstrs[(int) V_OWRITE]   = LGET("Okay to overwrite?") ;
  vstrs[(int) V_RANGE]    = LGET("Must be in the range 0 - 9") ;
  /* Do not translate this because this is resource file value */
  vstrs[(int) V_TRUE]     = DGET("true") ;
  vstrs[(int) V_UCON]     = LGET("Constant") ;
  vstrs[(int) V_UFUN]     = LGET("Function") ;
}
