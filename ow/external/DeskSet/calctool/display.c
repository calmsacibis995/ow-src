#ifndef lint
static char sccsid[] = "@(#)display.c 1.2 92/10/13 Copyr 1987 Sun Micro";
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
#include <math.h>
#include "color.h"
#include "calctool.h"
#include "extern.h"


int
char_val(chr)
char chr ;
{
       if (chr >= '0' && chr <= '9') return(chr - '0') ;
  else if (chr >= 'a' && chr <= 'f') return(chr - 'a' + 10) ;
  else if (chr >= 'A' && chr <= 'F') return(chr - 'A' + 10) ;
  else return(-1) ;
}


void
clear_display()
{
  int i ;

  v->pointed = 0 ;
  v->toclear = 1 ;
  i = 0 ;
  mpcim(&i, v->MPdisp_val) ;
  STRCPY(v->display, make_number(v->MPdisp_val)) ;
  set_item(DISPLAYITEM, v->display) ;

  v->hyperbolic = 0 ;
  v->inverse    = 0 ;
  v->show_paren = 0 ;
  v->opsptr     = 0 ;            /* Clear parentheses stacks. */
  v->numsptr    = 0 ;
  set_item(HYPITEM, "    ") ;
  set_item(INVITEM, "    ") ;
}


void
get_label(n, spaces, nchars)
int n, *spaces, *nchars ;
{
  int i, len, val ;

  val = button_value(n) ;
  if (v->tstate)
    switch (val)
        {
          case CTL('a') :
          case CTL('b') :
          case CTL('c') :
          case CTL('d') :
          case CTL('f') :
          case CTL('i') :
          case CTL('r') :
          case CTL('s') :
          case CTL('t') :
          case CTL('u') :
          case CTL('y') : SPRINTF(v->pstr, "^%c  ", val + 96) ;
                          break ;
          case CTL('h') : STRCPY(v->pstr, vstrs[(int) V_BSP]) ;
                          break ;
          case '\177'   : STRCPY(v->pstr, vstrs[(int) V_DEL]) ;
                          break ;
          default       : SPRINTF(v->pstr, "%c   ", val) ;
        }
  else STRCPY(v->pstr, button_str(n)) ;
  *spaces = 0 ;
  len = strlen(v->pstr) ;
  for (i = len-1; i >= 0; i--)
    if (v->pstr[i] == ' ') (*spaces)++ ;
    else break ;
  *nchars = len - *spaces ;
}


void
initialise()
{
  int i ;

  v->error         = 0 ;           /* Currently no display error. */
  v->cur_op        = '?' ;         /* No arithmetic operator defined yet. */
  v->old_cal_value = '?' ;
  i = 0 ;
  mpcim(&i, v->MPresult) ;         /* No previous result yet. */
  mpcim(&i, v->MPlast_input) ;
}


char *
make_fixed(MPnumber, cmax)     /* Convert MP number to fixed number string. */
int *MPnumber ;
int cmax ;                     /* Maximum characters to generate. */
{
  char *optr ;
  int MP1base[MP_SIZE], MP1[MP_SIZE], MP2[MP_SIZE], MPval[MP_SIZE] ;
  int ndig ;                   /* Total number of digits to generate. */
  int ddig ;                   /* Number of digits to left of . */
  int dval, n ;
 
  optr = v->fnum ;
  mpabs(MPnumber, MPval) ;
  n = 0 ;
  mpcim(&n, MP1) ;
  if (mplt(MPnumber, MP1)) *optr++ = '-' ;

  mpcim(&basevals[(int) v->base], MP1base) ;

  mppwr(MP1base, &v->accuracy, MP1) ;
  MPstr_to_num("0.5", DEC, MP2) ;
  mpdiv(MP2, MP1, MP1) ;
  mpadd(MPval, MP1, MPval) ;

  n = 1 ;
  mpcim(&n, MP2) ;
  if (mplt(MPval, MP2))
    {
      ddig = 0 ;
      *optr++ = '0' ;
      cmax-- ;
    }
  else
    for (ddig = 0; mpge(MPval, MP2); ddig++)
      mpdiv(MPval, MP1base, MPval) ;
 
  ndig = MIN(ddig + v->accuracy, --cmax) ;

  while (ndig-- > 0)
    {
      if (ddig-- == 0) *optr++ = '.' ;
      mpmul(MPval, MP1base, MPval) ;
      mpcmi(MPval, &dval) ;
      *optr++ = digits[dval] ;
      dval = -dval ;
      mpaddi(MPval, &dval, MPval) ;
    }    
  *optr++    = '\0' ;
  v->toclear = 1 ;
  v->pointed = 0 ;
  return(v->fnum) ;
}


char *
make_number(MPnumber)     /* Convert MP number to character string. */
int *MPnumber ;
{
  double number, val ;

/*  NOTE: make_number can currently set v->error when converting to a double.
 *        This is to provide the same look&feel as V3 even though calctool
 *        now does internal arithmetic to "infinite" precision.
 *
 *  XXX:  Needs to be improved. Shouldn't need to convert to a double in
 *        order to do these tests.
 */

  mpcmd(MPnumber, &number) ;
  val = fabs(number) ;
  if (v->error) return(vstrs[(int) V_ERROR]) ;
  if (v->dtype == ENG ||
      v->dtype == SCI ||
      v->dtype == FIX && val != 0.0 && (val > max_fix[(int) v->base]))
    return(make_eng_sci(MPnumber)) ;
  else return(make_fixed(MPnumber, MAX_DIGITS)) ;
}


char *
make_eng_sci(MPnumber)      /* Convert engineering or scientific number. */
int *MPnumber ;
{
  char fixed[MAX_DIGITS+1], *optr ;
  int MP1[MP_SIZE], MPatmp[MP_SIZE], MPval[MP_SIZE] ;
  int MP1base[MP_SIZE], MP3base[MP_SIZE], MP10base[MP_SIZE] ;
  int i, dval, len, n ;
  int MPmant[MP_SIZE] ;        /* Mantissa. */
  int ddig ;                   /* Number of digits in exponent. */
  int eng = 0 ;                /* Set if this is an engineering number. */
  int exp = 0 ;                /* Exponent */

  if (v->dtype == ENG) eng = 1 ;
  optr = v->snum ;
  mpabs(MPnumber, MPval) ;
  n = 0 ;
  mpcim(&n, MP1) ;
  if (mplt(MPnumber, MP1)) *optr++ = '-' ;
  mpstr(MPval, MPmant) ;

  mpcim(&basevals[(int) v->base], MP1base) ;
  n = 3 ;
  mppwr(MP1base, &n, MP3base) ;

  n = 10 ;
  mppwr(MP1base, &n, MP10base) ;

  n = 1 ;
  mpcim(&n, MP1) ;
  mpdiv(MP1, MP10base, MPatmp) ;

  n = 0 ;
  mpcim(&n, MP1) ;
  if (!mpeq(MPmant, MP1))
    {
      while (mpge(MPmant, MP10base))
        {
          exp += 10 ;
          mpmul(MPmant, MPatmp, MPmant) ;
        }
 
      while ((!eng &&  mpge(MPmant, MP1base)) ||
              (eng && (mpge(MPmant, MP3base) || exp % 3 != 0)))
        {
          exp += 1 ;
          mpdiv(MPmant, MP1base, MPmant) ;
        }
 
      while (mplt(MPmant, MPatmp))
        {
          exp -= 10 ;
          mpmul(MPmant, MP10base, MPmant) ;
        }
 
      n = 1 ;
      mpcim(&n, MP1) ;
      while (mplt(MPmant, MP1) || (eng && exp % 3 != 0))
        {
          exp -= 1 ;
          mpmul(MPmant, MP1base, MPmant) ;
        }
    }
 
  STRCPY(fixed, make_fixed(MPmant, MAX_DIGITS-6)) ;
  len = strlen(fixed) ;
  for (i = 0; i < len; i++) *optr++ = fixed[i] ;
 
  *optr++ = 'e' ;
 
  if (exp < 0)
    {
      exp = -exp ;
      *optr++ = '-' ;
    }
  else *optr++ = '+' ;
 
  MPstr_to_num("0.5", DEC, MP1) ;
  mpaddi(MP1, &exp, MPval) ;
  n = 1 ;
  mpcim(&n, MP1) ;
  for (ddig = 0; mpge(MPval, MP1); ddig++)
    mpdiv(MPval, MP1base, MPval) ;
 
  if (ddig == 0) *optr++ = '0' ;
 
  while (ddig-- > 0)
    {
      mpmul(MPval, MP1base, MPval) ;
      mpcmi(MPval, &dval) ;
      *optr++ = digits[dval] ;
      dval = -dval ;
      mpaddi(MPval, &dval, MPval) ;
    }
  *optr++    = '\0' ;
  v->toclear = 1 ;
  v->pointed = 0 ;
  return(v->snum) ;
}


void
MPstr_to_num(str, base, MPval)    /* Convert string into an MP number. */
char *str ;
enum base_type base ;
int *MPval ;
{
  char   *optr ;
  int MP1[MP_SIZE], MP2[MP_SIZE], MPbase[MP_SIZE] ;
  int    i, inum ;
  int    exp      = 0 ;
  int    exp_sign = 1 ;

  i = 0 ;
  mpcim(&i, MPval) ;
  mpcim(&basevals[(int) base], MPbase) ;
  optr = str ;
  while ((inum = char_val(*optr)) >= 0)
    {
      mpmul(MPval, MPbase, MPval) ;
      mpaddi(MPval, &inum, MPval) ;
      optr++ ;
    }

  if (*optr == '.')
    for (i = 1; (inum = char_val(*++optr)) >= 0; i++)
      {
        mppwr(MPbase, &i, MP1) ;
        mpcim(&inum, MP2) ;
        mpdiv(MP2, MP1, MP1) ;
        mpadd(MPval, MP1, MPval) ;
      }

  while (*optr == ' ') optr++ ;
 
  if (*optr != '\0')
    {
      if (*optr == '-') exp_sign = -1 ;
 
      while ((inum = char_val(*++optr)) >= 0)
        exp = exp * basevals[(int) base] + inum ;
    }
  exp *= exp_sign ;
     
  if (v->key_exp)
    {
      mppwr(MPbase, &exp, MP1) ;
      mpmul(MPval, MP1, MPval) ;
    }
}


void
paren_disp(c)   /* Append the latest parenthesis char to the display item. */
char c ;
{
  int i, n ;

/*  If the character is a Delete, clear the whole line, and exit parenthesis
 *  processing.
 *
 *  If the character is a Back Space, remove the last character. If the last
 *  character was a left parenthesis, decrement the parentheses count. If
 *  the parentheses count is zero, exit parenthesis processing.
 *
 *  If the character is a control character (not Ctrl-h), then append ^(char).
 *
 *  Otherwise just append the character.
 */

  n = strlen(v->display) ;
  if (IS_KEY(c, KEY_CLR))             /* Is it a Delete character? */
    {
      v->noparens = v->pending = v->opsptr = v->numsptr = 0 ;
      v->cur_op = '?' ;
      set_item(OPITEM, "") ;
      i = 0 ;
      mpcim(&i, v->MPdisp_val) ;
      show_display(v->MPdisp_val) ;
      return ;
    }
  else if (IS_KEY(c, KEY_BSP))        /* Is is a Back Space character? */
    {
      if (!n) return ;
      if (v->display[n-1] == '(')
        {
          v->noparens-- ;
          if (!v->noparens)
            {
              v->pending = v->opsptr = v->numsptr = 0 ;
              v->cur_op = '?' ;
              set_item(OPITEM, "") ;
              show_display(v->MPdisp_val) ;
              return ;
            }
        }
      v->display[n-1] = '\0' ;
    }
  else if (c <= CTL('z'))             /* Is it a control character? */
    {
      if (n < MAXLINE-2)
        {
          v->display[n]   = '^' ;
          v->display[n+1] = c + 96 ;
          v->display[n+2] = '\0' ;
        }
    }
  else                                /* It must be an ordinary character. */
    {
      if (n < MAXLINE-1)
        {
          v->display[n]   = c ;
          v->display[n+1] = '\0' ;
        }
    }

  n = (n < MAX_DIGITS) ? 0 : n - MAX_DIGITS ;
  v->show_paren = 1 ;       /* Hack to get set_item to really display it. */
  set_item(DISPLAYITEM, &v->display[n]) ;
  v->show_paren = 0 ;
}


void
process_item(n)
int n ;
{
  int i,isvalid ;

  if (n < 0 || n >= TITEMS)
    {
      if (v->beep == TRUE) beep() ;
      return ;
    }

  v->current = button_value(n) ;
  if (v->current == '*') v->current = 'x' ;      /* Reassign "extra" values. */
  if (v->current == '\015') v->current = '=' ;
  if (v->current == 'Q') v->current = 'q' ;

  if (v->error)
    {
      isvalid = 0 ;                    /* Must press a valid key first. */
      for (i = 0; i < MAXVKEYS; i++)
        if (v->current == validkeys[i]) isvalid = 1 ;
      if (v->pending == '?') isvalid = 1 ;
      if (!isvalid) return ;
      v->error = 0 ;
    }

  if (v->pending)
    {
      if (v->pending_win == FCP_KEY) (*buttons[v->pending_n].func)() ;
      else (*mode_buttons[MODEKEYS * ((int) v->pending_mode - 1) +
            v->pending_n].func)() ;
      return ; 
    }
  STRCPY(v->opstr, v->item_text[(int) OPITEM]) ;
  switch (button_opdisp(n))
    {
      case OP_SET   : set_item(OPITEM, button_str(n)) ;
                      break ;
      case OP_CLEAR : if (v->error) set_item(OPITEM, vstrs[(int) V_CLR]) ;
                            else set_item(OPITEM, "") ;
    }
  if (v->curwin == FCP_KEY) (*buttons[n].func)() ;
  else (*mode_buttons[MODEKEYS * ((int) v->modetype - 1) + n].func)() ;
}


void
show_display(MPval)
int *MPval ;
{
  if (!v->error)
    {
      STRCPY(v->display, make_number(MPval)) ;
      set_item(DISPLAYITEM, v->display) ;
    }
}


void
switch_hands(righthand)
int righthand ;
{
  int i, j, n ;

  for (i = 0; i < BROWS; i++)
    {
      for (j = 0; j < BCOLS; j++)
        {
          n = (righthand) ? right_pos[j] : left_pos[j] ;
           MEMCPY((char *) &v->temp_buttons[n],
		(char *) &buttons[i*BCOLS + j], sizeof(struct button)) ;
        }
      for (j = 0; j < BCOLS; j++)
         MEMCPY((char *) &buttons[i*BCOLS + j], 
		(char *) &v->temp_buttons[j], sizeof(struct button)) ;
    }
}
