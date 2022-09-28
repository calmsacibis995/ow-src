#ifndef lint
static char sccsid[] = "@(#)functions.c 1.10 93/07/06 Copyr 1987 Sun Micro";
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
#include <errno.h>
#include <string.h>
#include <math.h>
#include "color.h"
#include "calctool.h"
#include "extern.h"


void
do_accuracy()     /* Set display accuracy. */
{
  int i ;

  for (i = ACC_START; i <= ACC_END; i++)
    if (v->current == menu_entries[i])
      {
        v->accuracy = char_val(v->current) ;
        make_registers() ;
        return ;
      }
}


void
do_ascii()        /* Convert ASCII value. */
{
  int val ;

  if (v->pending)
    {
      val = v->cur_ch ;
      mpcim(&val, v->MPdisp_val) ;
      show_display(v->MPdisp_val) ;
      set_item(OPITEM, "") ;
      v->pending = 0 ;
    }
  else if (v->event_type != KEYBOARD_DOWN) show_ascii_frame() ;
  else
    {
      save_pending_values(v->current) ;
      set_item(OPITEM, vstrs[(int) V_ASC]) ;
    }
}


void
do_base()    /* Change the current base setting. */
{
       if (v->current == BASE_BIN) v->base = BIN ;
  else if (v->current == BASE_OCT) v->base = OCT ;
  else if (v->current == BASE_DEC) v->base = DEC ;
  else if (v->current == BASE_HEX) v->base = HEX ;
  else return ;

  grey_buttons(v->base) ;
  set_item(BASEITEM, base_str[(int) v->base]) ;
  show_display(v->MPdisp_val) ;
  v->pending = 0 ;
  if (v->rstate) make_registers() ;
}


void
do_business()     /* Perform special business mode calculations. */
{
  int MPbv[MP_SIZE], MP1[MP_SIZE], MP2[MP_SIZE], MP3[MP_SIZE], MP4[MP_SIZE] ;
  int i, len, val ;

  if (IS_KEY(v->current, KEY_CTRM))
    {

/*  Cterm - MEM0 = int (periodic interest rate).
 *          MEM1 = fv  (future value).
 *          MEM2 = pv  (present value).
 *
 *          RESULT = log(MEM1 / MEM2) / log(1 + MEM0)
 */

      mpdiv(v->MPmvals[1], v->MPmvals[2], MP1) ;
      mpln(MP1, MP2) ;
      val = 1 ;
      mpaddi(v->MPmvals[0], &val, MP3) ;
      mpln(MP3, MP4) ;
      mpdiv(MP2, MP4, v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_DDB))
    {

/*  Ddb   - MEM0 = cost    (amount paid for asset).
 *          MEM1 = salvage (value of asset at end of its life).
 *          MEM2 = life    (useful life of the asset).
 *          MEM3 = period  (time period for depreciation allowance).
 *
 *          bv = 0.0 ;
 *          for (i = 0; i < MEM3; i++)
 *            {
 *              VAL = ((MEM0 - bv) * 2) / MEM2
 *              bv += VAL
 *            }
 *          RESULT = VAL
 */

      i = 0 ;
      mpcim(&i, MPbv) ;
      mpcmi(v->MPmvals[3], &len) ;
      for (i = 0; i < len; i++)
        {
          mpsub(v->MPmvals[0], MPbv, MP1) ;
          val = 2 ;
          mpmuli(MP1, &val, MP2) ;
          mpdiv(MP2, v->MPmvals[2], v->MPdisp_val) ;
          mpstr(MPbv, MP1) ;
          mpadd(MP1, v->MPdisp_val, MPbv) ;
        }
    }
  else if (IS_KEY(v->current, KEY_FV))
    {

/*  Fv    - MEM0 = pmt (periodic payment).
 *          MEM1 = int (periodic interest rate).
 *          MEM2 = n   (number of periods).
 *
 *          RESULT = MEM0 * (pow(1 + MEM1, MEM2) - 1) / MEM1
 */

      val = 1 ;
      mpaddi(v->MPmvals[1], &val, MP1) ;
      mppwr2(MP1, v->MPmvals[2], MP2) ;
      val = -1 ;
      mpaddi(MP2, &val, MP3) ;
      mpmul(v->MPmvals[0], MP3, MP4) ;
      mpdiv(MP4, v->MPmvals[1], v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_PMT))
    {

/*  Pmt   - MEM0 = prin (principal).
 *          MEM1 = int  (periodic interest rate).
 *          MEM2 = n    (term).
 *
 *          RESULT = MEM0 * (MEM1 / (1 - pow(MEM1 + 1, -1 * MEM2)))
 */

      val = 1 ;
      mpaddi(v->MPmvals[1], &val, MP1) ;
      val = -1 ;
      mpmuli(v->MPmvals[2], &val, MP2) ;
      mppwr2(MP1, MP2, MP3) ;
      val = -1 ;
      mpmuli(MP3, &val, MP4) ;
      val = 1 ;
      mpaddi(MP4, &val, MP1) ;
      mpdiv(v->MPmvals[1], MP1, MP2) ;
      mpmul(v->MPmvals[0], MP2, v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_PV))
    {

/*  Pv    - MEM0 = pmt (periodic payment).
 *          MEM1 = int (periodic interest rate).
 *          MEM2 = n   (term).
 *
 *          RESULT = MEM0 * (1 - pow(1 + MEM1, -1 * MEM2)) / MEM1
 */

      val = 1 ;
      mpaddi(v->MPmvals[1], &val, MP1) ;
      val = -1 ;
      mpmuli(v->MPmvals[2], &val, MP2) ;
      mppwr2(MP1, MP2, MP3) ;
      val = -1 ;
      mpmuli(MP3, &val, MP4) ;
      val = 1 ;
      mpaddi(MP4, &val, MP1) ;
      mpdiv(MP1, v->MPmvals[1], MP2) ;
      mpmul(v->MPmvals[0], MP2, v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_RATE))
    {

/*  Rate  - MEM0 = fv (future value).
 *          MEM1 = pv (present value).
 *          MEM2 = n  (term).
 *
 *          RESULT = pow(MEM0 / MEM1, 1 / MEM2) - 1
 */

      mpdiv(v->MPmvals[0], v->MPmvals[1], MP1) ;
      val = 1 ;
      mpcim(&val, MP2) ;
      mpdiv(MP2, v->MPmvals[2], MP3) ;
      mppwr2(MP1, MP3, MP4) ;
      val = -1 ;
      mpaddi(MP4, &val, v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_SLN))
    {

/*  Sln   - MEM0 = cost    (cost of the asset).
 *          MEM1 = salvage (salvage value of the asset).
 *          MEM2 = life    (useful life of the asset).
 *
 *          RESULT = (MEM0 - MEM1) / MEM2
 */

      mpsub(v->MPmvals[0], v->MPmvals[1], MP1) ;
      mpdiv(MP1, v->MPmvals[2], v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_SYD))
    {

/*  Syd   - MEM0 = cost    (cost of the asset).
 *          MEM1 = salvage (salvage value of the asset).
 *          MEM2 = life    (useful life of the asset).
 *          MEM3 = period  (period for which depreciation is computed).
 *
 *          RESULT = ((MEM0 - MEM1) * (MEM2 - MEM3 + 1)) /
 *                   (MEM2 * (MEM2 + 1) / 2)
 */

      mpsub(v->MPmvals[2], v->MPmvals[3], MP2) ;
      val = 1 ;
      mpaddi(MP2, &val, MP3) ;
      mpaddi(v->MPmvals[2], &val, MP2) ;
      mpmul(v->MPmvals[2], MP2, MP4) ;
      val = 2 ;
      mpcim(&val, MP2) ;
      mpdiv(MP4, MP2, MP1) ;
      mpdiv(MP3, MP1, MP2) ;
      mpsub(v->MPmvals[0], v->MPmvals[1], MP1) ;
      mpmul(MP1, MP2, v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_TERM))
    {

/*  Term  - MEM0 = pmt (periodic payment).
 *          MEM1 = fv  (future value).
 *          MEM2 = int (periodic interest rate).
 *
 *          RESULT = log(1 + (MEM1 * MEM2 / MEM0)) / log(1 + MEM2)
 */

      val = 1 ;
      mpaddi(v->MPmvals[2], &val, MP1) ;
      mpln(MP1, MP2) ;
      mpmul(v->MPmvals[1], v->MPmvals[2], MP1) ;
      mpdiv(MP1, v->MPmvals[0], MP3) ;
      val = 1 ;
      mpaddi(MP3, &val, MP4) ;
      mpln(MP4, MP1) ;
      mpdiv(MP1, MP2, v->MPdisp_val) ;
    }
  show_display(v->MPdisp_val) ;
}


void
do_calc()      /* Perform arithmetic calculation and display result. */
{
  double dval, dres ;
  int MP1[MP_SIZE] ;

  if (!(v->opsptr && !v->show_paren))   /* Don't do if processing parens. */
    if (IS_KEY(v->current, KEY_EQ) && IS_KEY(v->old_cal_value, KEY_EQ))
      if (v->new_input) mpstr(v->MPlast_input, v->MPresult) ;
      else              mpstr(v->MPlast_input, v->MPdisp_val) ;

  if (!IS_KEY(v->current, KEY_EQ) && IS_KEY(v->old_cal_value, KEY_EQ))
    v->cur_op = '?' ;

  if (IS_KEY(v->cur_op, KEY_COS) ||                           /* Cos */
      IS_KEY(v->cur_op, KEY_SIN) ||                           /* Sin */
      IS_KEY(v->cur_op, KEY_TAN) ||                           /* Tan */
      v->cur_op == '?')                                 /* Undefined */
    mpstr(v->MPdisp_val, v->MPresult) ;

  else if (IS_KEY(v->cur_op, KEY_ADD))                  /* Addition */
    mpadd(v->MPresult, v->MPdisp_val, v->MPresult) ;

  else if (IS_KEY(v->cur_op, KEY_SUB))                  /* Subtraction. */
    mpsub(v->MPresult, v->MPdisp_val, v->MPresult) ;

  else if (v->cur_op == '*' ||
           IS_KEY(v->cur_op, KEY_MUL))                  /* Multiplication */
    mpmul(v->MPresult, v->MPdisp_val, v->MPresult) ;

  else if (IS_KEY(v->cur_op, KEY_DIV))                  /* Division. */
    mpdiv(v->MPresult, v->MPdisp_val, v->MPresult) ;

  else if (IS_KEY(v->cur_op, KEY_PER))                  /* % */
    {
      mpmul(v->MPresult, v->MPdisp_val, v->MPresult) ;
      MPstr_to_num("0.01", DEC, MP1) ;
      mpmul(v->MPresult, MP1, v->MPresult) ;
    }

  else if (IS_KEY(v->cur_op, KEY_YTOX))                 /* y^x */
    mppwr2(v->MPresult, v->MPdisp_val, v->MPresult) ;

  else if (IS_KEY(v->cur_op, KEY_AND))                  /* And */
    {
      mpcmd(v->MPresult, &dres) ;
      mpcmd(v->MPdisp_val, &dval) ;
      dres = setbool(ibool(dres) & ibool(dval)) ;
      mpcdm(&dres, v->MPresult) ;
    }

  else if (IS_KEY(v->cur_op, KEY_OR))                   /* Or */
    {
      mpcmd(v->MPresult, &dres) ;
      mpcmd(v->MPdisp_val, &dval) ;
      dres = setbool(ibool(dres) | ibool(dval)) ;
      mpcdm(&dres, v->MPresult) ;
    }

  else if (IS_KEY(v->cur_op, KEY_XOR))                  /* Xor */
    {
      mpcmd(v->MPresult, &dres) ;
      mpcmd(v->MPdisp_val, &dval) ;
      dres = setbool(ibool(dres) ^ ibool(dval)) ;
      mpcdm(&dres, v->MPresult) ; 
    }

  else if (IS_KEY(v->cur_op, KEY_XNOR))                 /* Xnor */
    {
      mpcmd(v->MPresult, &dres) ;
      mpcmd(v->MPdisp_val, &dval) ;
      dres = setbool(~ibool(dres) ^ ibool(dval)) ;
      mpcdm(&dres, v->MPresult) ; 
    }

  else if (IS_KEY(v->cur_op, KEY_EQ)) /* do nothing */ ;      /* Equals */

  show_display(v->MPresult) ;

  if (!(IS_KEY(v->current, KEY_EQ) && IS_KEY(v->old_cal_value, KEY_EQ)))
    mpstr(v->MPdisp_val, v->MPlast_input) ;

  mpstr(v->MPresult, v->MPdisp_val) ;
  if (!IS_KEY(v->current, KEY_EQ)) v->cur_op = v->current ;
  v->old_cal_value = v->current ;
  v->new_input     = v->key_exp = 0 ;
}


void
do_clear()       /* Clear the calculator display and re-initialise. */
{
  clear_display() ;
  if (v->error) set_item(DISPLAYITEM, "") ;
  initialise() ;
}


void
do_constant()
{
       if (!v->current) start_popup(M_CON) ;
  else if (v->current >= '0' && v->current <= '9')
    {
      mpstr(v->MPcon_vals[char_val(v->current)], v->MPdisp_val) ;
      show_display(v->MPdisp_val) ;
    }
}


void
do_delete()     /* Remove the last numeric character typed. */
{
  if (strlen(v->display))
    v->display[strlen(v->display)-1] = '\0' ;

/*  If we were entering a scientific number, and we have backspaced over
 *  the exponent sign, then this reverts to entering a fixed point number.
 */

  if (v->key_exp && !(strchr(v->display, '+')))
    {
      v->key_exp = 0 ;
      v->display[strlen(v->display)-1] = '\0' ;
      set_item(OPITEM, "") ;
    }

/* If we've backspaced over the numeric point, clear the pointed flag. */

  if (v->pointed && !(strchr(v->display, '.'))) v->pointed = 0 ;

  set_item(DISPLAYITEM, v->display) ;
  MPstr_to_num(v->display, v->base, v->MPdisp_val) ;
}


void
do_exchange()         /* Exchange display with memory register. */
{
  int i, MPtemp[MP_SIZE] ;

  for (i = MEM_START; i <= MEM_END; i++)
    if (v->current == menu_entries[i])
      {
        mpstr(v->MPdisp_val, MPtemp) ;
        mpstr(v->MPmvals[char_val(v->current)], v->MPdisp_val) ;
        mpstr(MPtemp, v->MPmvals[char_val(v->current)]) ;
        make_registers() ;
        return ;
      }
}


void
do_expno()           /* Get exponential number. */
{
  v->pointed = (strchr(v->display, '.') != NULL) ;
  if (!v->new_input)
    {
      STRCPY(v->display, "1.0 +") ;
      v->new_input = v->pointed = 1 ;
    }
  else if (!v->pointed)
    {
      STRNCAT(v->display, ". +", 3) ;
      v->pointed = 1 ;
    } 
  else if (!v->key_exp) STRNCAT(v->display, " +", 2) ;
  v->toclear = 0 ;
  v->key_exp = 1 ;
  v->exp_posn = strchr(v->display, '+') ;
  set_item(DISPLAYITEM, v->display) ;
  MPstr_to_num(v->display, v->base, v->MPdisp_val) ;
}


void
do_factorial(MPval, MPres)             /* Calculate the factorial of MPval. */
int *MPval, *MPres ;
{
  double val ;
  int i, MPa[MP_SIZE], MP1[MP_SIZE], MP2[MP_SIZE] ; 
 
/*  NOTE: do_factorial, on each iteration of the loop, will attempt to
 *        convert the current result to a double. If v->error is set,
 *        then we've overflowed. This is to provide the same look&feel
 *        as V3.
 *
 *  XXX:  Needs to be improved. Shouldn't need to convert to a double in
 *        order to check this.
 */

  mpstr(MPval, MPa) ;
  mpcmim(MPval, MP1) ;
  i = 0 ;
  mpcim(&i, MP2) ;
  if (mpeq(MPval, MP1) &&  mpge(MPval, MP2))  /* Only positive integers. */
    {
      i = 1 ;
      if (mpeq(MP1, MP2))                     /* Special case for 0! */
        {
          mpcim(&i, MPres) ;
          return ;
        }
      mpcim(&i, MPa) ;
      mpcmi(MP1, &i) ;
      if (!i) matherr((struct exception *) NULL) ;
      else
        while (i > 0)
          {
            mpmuli(MPa, &i, MPa) ;
            mpcmd(MPa, &val) ;
            if (v->error) break ;
            i-- ;
          }
    } 
  else matherr((struct exception *) NULL) ;
  mpstr(MPa, MPres) ;
}


void
do_frame()    /* Exit calctool. */
{
  exit(0) ;
}


void
do_function()      /* Perform a user defined function. */
{
  enum fcp_type scurwin ;
  int fno, scolumn, srow ;

  srow = v->row ;
  scolumn = v->column ;
  scurwin = v->curwin ;
  v->pending = 0 ;
       if (!v->current) start_popup(M_FUN) ;
  else if (v->current >= '0' && v->current <= '9')
    {
      fno = char_val(v->current) ;
      process_str(v->fun_vals[fno]) ;
    }
  v->curwin = scurwin ;
  v->row = srow ;
  v->column = scolumn ;
}


void
do_help()            /* F1 key was pressed; show help message. */
{
  char help_str[MAXLINE] ;  /* Passed to the help display routine. */
  int column, n, row ;

  SPRINTF(help_str, "%s:", "calctool") ;
  if (v->cury <= v->ndisplay && v->curwin == FCP_KEY)  /* Help over display? */
    {
      STRCAT(help_str, vstrs[(int) V_DISP]) ;
      show_help(help_str) ;
    }
  else                      /* Check for help over buttons. */
    {
      if (v->curwin == FCP_KEY) v->cury -= v->ndisplay ;
      for (row = 0; row < MAXROWS; row++)
        for (column = 0; column < MAXCOLS; column++)
          if ((v->curx > (column * (v->bwidth  + v->bgap) + v->bborder)) &&
              (v->curx < (column * (v->bwidth  + v->bgap) + v->bborder +
                                                            v->bwidth)) &&
              (v->cury > (row    * (v->bheight + v->bgap) + v->bborder)) &&
              (v->cury < (row    * (v->bheight + v->bgap) + v->bborder +
                                                            v->bheight)))
            {
              n = row * MAXCOLS + column ;
              if (button_bstate(n) != B_NULL)
                STRCAT(help_str, help_button_str(row * MAXCOLS + column)) ;
              else STRCAT(help_str, vstrs[(int) V_CANVAS]) ;
              show_help(help_str) ;
              return ;
            }
      STRCAT(help_str, vstrs[(int) V_CANVAS]) ;
      show_help(help_str) ;
    }
}


void
do_immed()
{
  double dval ;
  int i, MP1[MP_SIZE], MP2[MP_SIZE] ;

       if (IS_KEY(v->current, KEY_HYP))          /* Hyp */
    {
      v->hyperbolic = !v->hyperbolic ;
      set_item(HYPITEM, (v->hyperbolic) ? vstrs[(int) V_HYP]
                                        : "    ") ;
    }

  else if (IS_KEY(v->current, KEY_INV))          /* Inv */
    {
      v->inverse = !v->inverse ;
      set_item(INVITEM, (v->inverse) ? vstrs[(int) V_INV]
                                     : "    ") ;
    }

  else if (IS_KEY(v->current, KEY_32))           /* &32 */
    {
      mpcmd(v->MPdisp_val, &dval) ;
      dval = setbool(ibool(dval)) ;
      mpcdm(&dval, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_16))           /* &16 */
    {
      mpcmd(v->MPdisp_val, &dval) ;
      dval = setbool(ibool(dval) & 0xffff) ;
      mpcdm(&dval, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_ETOX))         /* e^x */
    {
      mpstr(v->MPdisp_val, MP1) ;
      mpexp(MP1, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_TTOX))         /* 10^x */
    {
      i = 10 ;
      mpcim(&i, MP1) ;
      mppwr2(MP1, v->MPdisp_val, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_LN))           /* Ln */
    {
      mpstr(v->MPdisp_val, MP1) ;
      mpln(MP1, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_LOG))          /* Log */
    {
      mplog10(v->MPdisp_val, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_RAND))         /* Rand */
    {
      dval = drand48() ;
      mpcdm(&dval, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_SQRT))         /* Sqrt */
    {
      mpstr(v->MPdisp_val, MP1) ;
      mpsqrt(MP1, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_NOT))          /* Not */
    {
      mpcmd(v->MPdisp_val, &dval) ;
      dval = setbool(~ibool(dval)) ;               
      mpcdm(&dval, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_REC))          /* 1/x */
    {
      i = 1 ;
      mpcim(&i, MP1) ;
      mpstr(v->MPdisp_val, MP2) ;
      mpdiv(MP1, MP2, v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_FACT))         /* x! */
    {
      do_factorial(v->MPdisp_val, MP1) ;
      mpstr(MP1, v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_SQR))          /* x^2 */
    {
      mpstr(v->MPdisp_val, MP1) ;
      mpmul(MP1, MP1, v->MPdisp_val) ;
    }

  else if (IS_KEY(v->current, KEY_CHS))          /* +/- */
    {
      if (v->key_exp)
        {
          if (*v->exp_posn == '+') *v->exp_posn = '-' ;
          else                     *v->exp_posn = '+' ;
          set_item(DISPLAYITEM, v->display) ;
          MPstr_to_num(v->display, v->base, v->MPdisp_val) ;
          v->key_exp = 0 ;
        }
      else mpneg(v->MPdisp_val, v->MPdisp_val) ;
    }
  show_display(v->MPdisp_val) ;
}


void
do_keys()      /* Display/undisplay the calctool key values. */
{
  enum fcp_type scurwin ;

  make_canvas(0, 0, v->twidth, v->theight, 1) ;
  if (v->modetype != BASIC)
    {
      scurwin   = v->curwin ;
      v->curwin = FCP_MODE ;
      make_modewin(0, 0, v->mwidth, v->mheight) ;
      v->curwin = scurwin ;
    }
}


void
do_memory()
{
  if (!v->rstate)
    {
      make_registers() ;
      v->rstate = 1 ;
    }
  win_display(FCP_REG, TRUE) ;
}


void
do_mode()                  /* Set special calculator mode. */
{
  enum fcp_type scurwin ;
  enum mode_type old ;     /* Previous mode setting. */

  old = v->modetype ;

  if (v->current == MODE_BAS)              /* Basic (none). */
    {
      v->modetype = BASIC ;
      set_item(MODEITEM, mode_str[(int) v->modetype]) ;
      win_display(FCP_MODE, FALSE) ;
      return ;
    }

  else if (v->current == MODE_FIN)         /* Financial. */
    {
      v->modetype = FINANCIAL ;
      set_title(FCP_MODE, mstrs[(int) FINANCIAL]) ;
    }

  else if (v->current == MODE_LOG)         /* Logical. */
    {
      v->modetype = LOGICAL ;
      set_title(FCP_MODE, mstrs[(int) LOGICAL]) ;
    }

  else if (v->current == MODE_SCI)         /* Scientific. */
    {
      v->modetype = SCIENTIFIC ;
      set_title(FCP_MODE, mstrs[(int) SCIENTIFIC]) ;
    }

  else return ;

  set_item(MODEITEM, mode_str[(int) v->modetype]) ;
  if (old == v->modetype || v->modetype == BASIC)
    {
      win_display(FCP_MODE, TRUE) ;
      return ;
    }

  scurwin = v->curwin ;
  v->curwin = FCP_MODE ;

/* Force repaint of mode buttons. */

  make_modewin(0, 0, v->mwidth, v->mheight) ;
  v->curwin = scurwin ;
  win_display(FCP_MODE, TRUE) ;
}


void
do_none()       /* Null routine for empty buttons. */
{
}


void
do_number()
{
  char nextchar ;
  int len, n ;
  static int maxvals[4] = { 1, 7, 9, 15 } ;

  nextchar = v->current ;
  n = v->current - '0' ;
  if (v->base == HEX && v->current >= 'a' && v->current <= 'f')
    {
      nextchar -= 32 ;             /* Convert to uppercase hex digit. */
      n = v->current - 'a' + 10 ;
    }
  if (n > maxvals[(int) v->base])
    {
      if (v->beep == TRUE) beep() ;
      return ;
    }

  if (v->toclear)
    {
      SPRINTF(v->display, "%c", nextchar) ;
      v->toclear = 0 ;
    }
  else
    {
      len = strlen(v->display) ;
      if (len < MAX_DIGITS)
        {
          v->display[len] = nextchar ;
          v->display[len+1] = '\0' ;
        }
    }
  set_item(DISPLAYITEM, v->display) ;
  MPstr_to_num(v->display, v->base, v->MPdisp_val) ;
  v->new_input = 1 ;
}


void
do_numtype()    /* Set number type (engineering, fixed or scientific). */
{
       if (v->current == DISP_ENG) v->dtype = ENG ;
  else if (v->current == DISP_FIX) v->dtype = FIX ;
  else if (v->current == DISP_SCI) v->dtype = SCI ;
  else return ;

  set_item(NUMITEM, dtype_str[(int) v->dtype]) ;
  v->pending = 0 ;
  show_display(v->MPdisp_val) ;
  if (v->rstate) make_registers() ;
}


void
do_paren()
{
  char *ptr ;

/*  Check to see if this is the first outstanding parenthesis. If so, and
 *  their is a current operation already defined, then add the current
 *  operation to the parenthesis expression being displayed.
 *  Increment parentheses count, and add the open paren to the expression.
 */

  if (IS_KEY(v->current, KEY_LPAR))
    {
      if (!v->noparens && v->cur_op != '?') paren_disp(v->cur_op) ;
      v->pending = v->current ;
      v->noparens++ ;
    }

/*  If we haven't had any left brackets yet, and this is a right bracket,
 *  then just ignore it.
 *  Decrement the bracket count. If the count is zero, then process the
 *  parenthesis expression.
 */

  else if (IS_KEY(v->current, KEY_RPAR))
    {
      if (!v->noparens) return ;
      v->noparens-- ;
      if (!v->noparens)
        {
          paren_disp(v->current) ;
          ptr = v->display ;
          while (*ptr != '(') ptr++ ;
          while (*ptr != NULL) process_parens(*ptr++) ;
          return ;
        }
    }
  paren_disp(v->current) ;
}


void
do_pending()
{

/*  Certain pending operations which are half completed, force the numeric
 *  keypad to be reshown (assuming they already aren't).
 *
 *  Con, Exch, Fun, Sto, Rcl and Acc    show buttons 0 - 9.
 *  < and >                             show buttons 0 - f.
 */

  if (!v->ismenu)
    {
      if (IS_KEY(v->current, KEY_CON)  ||      /* Con. */
          IS_KEY(v->current, KEY_EXCH) ||      /* Exch. */
          IS_KEY(v->current, KEY_FUN)  ||      /* Fun. */
          IS_KEY(v->current, KEY_STO)  ||      /* Sto. */
          IS_KEY(v->current, KEY_RCL)  ||      /* Rcl. */
          IS_KEY(v->current, KEY_ACC))         /* Acc. */
        grey_buttons(DEC) ;
      if (IS_KEY(v->current, KEY_LSFT) ||
          IS_KEY(v->current, KEY_RSFT))
        grey_buttons(HEX) ;
     }

       if (IS_KEY(v->pending, KEY_ASC))  do_ascii() ;        /* Asc */
  else if (IS_KEY(v->pending, KEY_BASE)) do_base() ;         /* Base */
  else if (IS_KEY(v->pending, KEY_DISP)) do_numtype() ;      /* Disp */
  else if (IS_KEY(v->pending, KEY_TRIG)) do_trigtype() ;     /* Trig */
  else if (IS_KEY(v->pending, KEY_CON))  do_constant() ;     /* Con */
  else if (IS_KEY(v->pending, KEY_EXCH)) do_exchange() ;     /* Exch */
  else if (IS_KEY(v->pending, KEY_FUN))  do_function() ;     /* Fun */
  else if (IS_KEY(v->pending, KEY_STO) ||                    /* Sto */
           IS_KEY(v->pending, KEY_RCL))                      /* Rcl */
    {
      do_sto_rcl() ;
      if (IS_KEY(v->pending_op, KEY_ADD) ||
          IS_KEY(v->pending_op, KEY_SUB) ||
          IS_KEY(v->pending_op, KEY_MUL) ||
          IS_KEY(v->pending_op, KEY_DIV)) return ;
    }
  else if (IS_KEY(v->pending, KEY_LSFT) ||                   /* < */
           IS_KEY(v->pending, KEY_RSFT)) do_shift() ;        /* > */
  else if (IS_KEY(v->pending, KEY_ACC))  do_accuracy() ;     /* Acc */
  else if (IS_KEY(v->pending, KEY_MODE)) do_mode() ;         /* Mode */
  else if (IS_KEY(v->pending, KEY_LPAR))                     /* ( */
    {
      do_paren() ;
      return ;
    }
  else if (!v->pending)
    {
      save_pending_values(v->current) ;
      v->pending_op = KEY_EQ ;
      return ;
    }

  show_display(v->MPdisp_val) ;
  if (v->error) set_item(OPITEM, vstrs[(int) V_CLR]) ;
  else set_item(OPITEM, v->opstr) ;  /* Redisplay pending op. (if any). */

  v->pending = 0 ;
  if (!v->ismenu)
    grey_buttons(v->base) ;  /* Just show numeric keys for current base. */
}


void
do_point()                   /* Handle numeric point. */
{
  if (!v->pointed)
    {
      if (v->toclear)
        {
          STRCPY(v->display, ".") ;
          v->toclear = 0 ;
        }
      else STRNCAT(v->display, ".", 1) ;
      v->pointed = 1 ;
    }
  set_item(DISPLAYITEM, v->display) ;
  MPstr_to_num(v->display, v->base, v->MPdisp_val) ;
}


void
do_portion()
{
  int MP1[MP_SIZE] ;

       if (IS_KEY(v->current, KEY_ABS))                      /* Abs */
    {
      mpstr(v->MPdisp_val, MP1) ;
      mpabs(MP1, v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_FRAC))                     /* Frac */
    {
      mpstr(v->MPdisp_val, MP1) ;
      mpcmf(MP1, v->MPdisp_val) ;
    }
  else if (IS_KEY(v->current, KEY_INT))                      /* Int */
    {
      mpstr(v->MPdisp_val, MP1) ;
      mpcmim(MP1, v->MPdisp_val) ;
    }
  show_display(v->MPdisp_val) ;
}


void
do_shift()     /* Perform bitwise shift on display value. */
{
  int i, MPtemp[MP_SIZE], shift ;
  BOOLEAN temp ;
  double dval ;

  for (i = SFT_START; i <= SFT_END; i++)
    if (v->current == menu_entries[i])
      {
        shift = char_val(v->current) ;
        MPstr_to_num(v->display, v->base, MPtemp) ;
        mpcmd(MPtemp, &dval) ;
        temp = ibool(dval) ;

             if (IS_KEY(v->pending, KEY_LSFT)) temp = temp << shift ;
        else if (IS_KEY(v->pending, KEY_RSFT)) temp = temp >> shift ;

        dval = setbool(temp) ;
        mpcdm(&dval, v->MPdisp_val) ;
        show_display(v->MPdisp_val) ;
        mpstr(v->MPdisp_val, v->MPlast_input) ;
        return ;
      }
}


void
do_sto_rcl()     /* Save/restore value to/from memory register. */
{
  int i, MPn[MP_SIZE], n ;

  for (i = MEM_START; i <= MEM_END; i++)
    if (v->current == menu_entries[i])
      {
        if (IS_KEY(v->pending, KEY_RCL))                        /* Rcl */
          {
            mpstr(v->MPmvals[char_val(v->current)], v->MPdisp_val) ;
            v->new_input = 0 ;
          }
        else if (IS_KEY(v->pending, KEY_STO))                   /* Sto */
          {
            n = char_val(v->current) ;

                 if (IS_KEY(v->pending_op, KEY_ADD))            /* + */
              {
                mpstr(v->MPmvals[n], MPn) ;
                mpadd(MPn, v->MPdisp_val, v->MPmvals[n]) ;
              }
            else if (IS_KEY(v->pending_op, KEY_SUB))            /* - */
              {
                mpstr(v->MPmvals[n], MPn) ;
                mpsub(MPn, v->MPdisp_val, v->MPmvals[n]) ;
              }
            else if (IS_KEY(v->pending_op, KEY_MUL))            /* x */
              {
                mpstr(v->MPmvals[n], MPn) ;
                mpmul(MPn, v->MPdisp_val, v->MPmvals[n]) ;
              }
            else if (IS_KEY(v->pending_op, KEY_DIV))            /* / */
              {
                mpstr(v->MPmvals[n], MPn) ;
                mpdiv(MPn, v->MPdisp_val, v->MPmvals[n]) ;
              }
            else mpstr(v->MPdisp_val, v->MPmvals[n]) ;

            v->pending_op = 0 ;
            make_registers() ;
          }
        return ;                 
      }

  if (IS_KEY(v->current, KEY_ADD) || IS_KEY(v->current, KEY_SUB) ||
      IS_KEY(v->current, KEY_MUL) || IS_KEY(v->current, KEY_DIV))
    v->pending_op = v->current ;
}


void
do_trig()         /* Perform all trigonometric functions. */
{
  int i, MPtemp[MP_SIZE], MP1[MP_SIZE], MP2[MP_SIZE] ;
  double cval ;
  int MPcos[MP_SIZE], MPsin[MP_SIZE] ;

  if (!v->inverse)
    {
      if (!v->hyperbolic)
        {
               if (v->ttype == DEG)
            {
              mppi(MP1) ;
              mpmul(v->MPdisp_val, MP1, MP2) ;
              i = 180 ;
              mpcim(&i, MP1) ;
              mpdiv(MP2, MP1, MPtemp) ;
            }
          else if (v->ttype == GRAD)
            {
              mppi(MP1) ;
              mpmul(v->MPdisp_val, MP1, MP2) ;
              i = 200 ;
              mpcim(&i, MP1) ;
              mpdiv(MP2, MP1, MPtemp) ;
            }
          else mpstr(v->MPdisp_val, MPtemp) ;
        }
      else mpstr(v->MPdisp_val, MPtemp) ;

      if (!v->hyperbolic)
        {
               if (IS_KEY(v->current, KEY_COS))                  /* Cos */
            mpcos(MPtemp, v->MPtresults[(int) RAD]) ;
          else if (IS_KEY(v->current, KEY_SIN))                  /* Sin */
            mpsin(MPtemp, v->MPtresults[(int) RAD]) ;
          else if (IS_KEY(v->current, KEY_TAN))                  /* Tan */
            {
              mpsin(MPtemp, MPsin) ;
              mpcos(MPtemp, MPcos) ;
              mpcmd(MPcos, &cval) ;
              if (cval == 0.0) doerr(vstrs[(int) V_ERROR]) ;
              mpdiv(MPsin, MPcos, v->MPtresults[(int) RAD]) ;
            }
        }
      else
        {
               if (IS_KEY(v->current, KEY_COS))                  /* Cosh */
            mpcosh(MPtemp, v->MPtresults[(int) RAD]) ;
          else if (IS_KEY(v->current, KEY_SIN))                  /* Sinh */
            mpsinh(MPtemp, v->MPtresults[(int) RAD]) ;
          else if (IS_KEY(v->current, KEY_TAN))                  /* Tanh */
            mptanh(MPtemp, v->MPtresults[(int) RAD]) ;
        }

      mpstr(v->MPtresults[(int) RAD], v->MPtresults[(int) DEG]) ;
      mpstr(v->MPtresults[(int) RAD], v->MPtresults[(int) GRAD]) ;
    }
  else
    {
      if (!v->hyperbolic)
        {
                 if (IS_KEY(v->current, KEY_COS))                /* Acos */
              mpacos(v->MPdisp_val, v->MPdisp_val) ;
            else if (IS_KEY(v->current, KEY_SIN))                /* Asin */
              mpasin(v->MPdisp_val, v->MPdisp_val) ;
            else if (IS_KEY(v->current, KEY_TAN))                /* Atan */
              mpatan(v->MPdisp_val, v->MPdisp_val) ;
        }
      else
        {
                 if (IS_KEY(v->current, KEY_COS))                /* Acosh */
              mpacosh(v->MPdisp_val, v->MPdisp_val) ;
            else if (IS_KEY(v->current, KEY_SIN))                /* Asinh */
              mpasinh(v->MPdisp_val, v->MPdisp_val) ;
            else if (IS_KEY(v->current, KEY_TAN))                /* Atanh */
              mpatanh(v->MPdisp_val, v->MPdisp_val) ;
        }

      if (!v->hyperbolic)
        {
          i = 180 ;
          mpcim(&i, MP1) ;
          mpmul(v->MPdisp_val, MP1, MP2) ;
          mppi(MP1) ;
          mpdiv(MP2, MP1, v->MPtresults[(int) DEG]) ;

          i = 200 ;
          mpcim(&i, MP1) ;
          mpmul(v->MPdisp_val, MP1, MP2) ;
          mppi(MP1) ;
          mpdiv(MP2, MP1, v->MPtresults[(int) GRAD]) ;
        }
      else
        {
          mpstr(v->MPdisp_val, v->MPtresults[(int) DEG]) ;
          mpstr(v->MPdisp_val, v->MPtresults[(int) GRAD]) ;
        }

      mpstr(v->MPdisp_val, v->MPtresults[(int) RAD]) ;
    }

  show_display(v->MPtresults[(int) v->ttype]) ;
  mpstr(v->MPtresults[(int) v->ttype], v->MPdisp_val) ;
}


void
do_trigtype()          /* Change the current trigonometric type. */
{
       if (v->current == TRIG_DEG) v->ttype = DEG ;
  else if (v->current == TRIG_GRA) v->ttype = GRAD ;
  else if (v->current == TRIG_RAD) v->ttype = RAD ;
  else return ;

  if (IS_KEY(v->cur_op, KEY_COS) ||
      IS_KEY(v->cur_op, KEY_SIN) ||
      IS_KEY(v->cur_op, KEY_TAN))
    {
      mpstr(v->MPtresults[(int) v->ttype], v->MPdisp_val) ;
      show_display(v->MPtresults[(int) v->ttype]) ;
    }
  set_item(TTYPEITEM, ttype_str[(int) v->ttype]) ;
  v->pending = 0 ;
}


BOOLEAN
ibool(x)
double x ;
{
  BOOLEAN p = (BOOLEAN) x ;

  return(p) ;
}


/*  The following MP routines were not in the Brent FORTRAN package. They are
 *  derived here, in terms of the existing routines.
 */

/*  MP precision arc cosine.
 *
 *  1. If (x < -1.0  or x > 1.0) then report DOMAIN error and return 0.0.
 *
 *  2. If (x = 0.0) then acos(x) = PI/2.
 *
 *  3. If (x = 1.0) then acos(x) = 0.0
 *
 *  4. If (x = -1.0) then acos(x) = PI.
 *
 *  5. If (0.0 < x < 1.0) then  acos(x) = atan(sqrt(1-(x**2)) / x)
 *
 *  6. If (-1.0 < x < 0.0) then acos(x) = atan(sqrt(1-(x**2)) / x) + PI
 */

void
mpacos(MPx, MPretval)
int *MPx, *MPretval ;
{
  int MP0[MP_SIZE],  MP1[MP_SIZE],  MP2[MP_SIZE] ;
  int MPn1[MP_SIZE], MPpi[MP_SIZE], MPy[MP_SIZE], val ;

  mppi(MPpi) ;
  val = 0 ;
  mpcim(&val, MP0) ;
  val = 1 ;
  mpcim(&val, MP1) ;
  val = -1 ;
  mpcim(&val, MPn1) ;

  if (mpgt(MPx, MP1) || mplt(MPx, MPn1))
    {
      doerr(LGET("acos DOMAIN error")) ;
      mpstr(MP0, MPretval) ;
    }
  else if (mpeq(MPx, MP0))
    {
      val = 2 ;
      mpdivi(MPpi, &val, MPretval) ;
    }
  else if (mpeq(MPx, MP1))  mpstr(MP0, MPretval) ;
  else if (mpeq(MPx, MPn1)) mpstr(MPpi, MPretval) ;
  else
    { 
      mpmul(MPx, MPx, MP2) ;
      mpsub(MP1, MP2, MP2) ;
      mpsqrt(MP2, MP2) ;
      mpdiv(MP2, MPx, MP2) ;
      mpatan(MP2, MPy) ;
      if (mpgt(MPx, MP0)) mpstr(MPy, MPretval) ;
      else                 mpadd(MPy, MPpi, MPretval) ;
    }
}


/*  MP precision hyperbolic arc cosine.
 *
 *  1. If (x < 1.0) then report DOMAIN error and return 0.0.
 *
 *  2. acosh(x) = log(x + sqrt(x**2 - 1))
 */

void
mpacosh(MPx, MPretval)
int *MPx, *MPretval ;
{
  int MP1[MP_SIZE], val ;

  val = 1 ;
  mpcim(&val, MP1) ;
  if (mplt(MPx, MP1))
    {
      doerr(LGET("acosh DOMAIN error")) ;
      val = 0 ;
      mpcim(&val, MPretval) ;
    }
  else
    {
      mpmul(MPx, MPx, MP1) ;
      val = -1 ;
      mpaddi(MP1, &val, MP1) ;
      mpsqrt(MP1, MP1) ;
      mpadd(MPx, MP1, MP1) ;
      mpln(MP1, MPretval) ;
    }
}


/*  MP precision hyperbolic arc sine.
 *
 *  1. asinh(x) = log(x + sqrt(x**2 + 1))
 */

void
mpasinh(MPx, MPretval)
int *MPx, *MPretval ;
{
  int MP1[MP_SIZE], val ;
 
  mpmul(MPx, MPx, MP1) ;
  val = 1 ;
  mpaddi(MP1, &val, MP1) ;
  mpsqrt(MP1, MP1) ;
  mpadd(MPx, MP1, MP1) ;
  mpln(MP1, MPretval) ;
}


/*  MP precision hyperbolic arc tangent.
 *
 *  1. If (x <= -1.0 or x >= 1.0) then report a DOMAIn error and return 0.0.
 *
 *  2. atanh(x) = 0.5 * log((1 + x) / (1 - x))
 */

void
mpatanh(MPx, MPretval)
int *MPx, *MPretval ;
{
  int MP0[MP_SIZE], MP1[MP_SIZE], MP2[MP_SIZE] ;
  int MP3[MP_SIZE], MPn1[MP_SIZE], val ;

  val = 0 ;
  mpcim(&val, MP0) ;
  val = 1 ;
  mpcim(&val, MP1) ;
  val = -1 ;
  mpcim(&val, MPn1) ;

  if (mpge(MPx, MP1) || mple(MPx, MPn1))
    {
      doerr(LGET("atanh DOMAIN error")) ;
      mpstr(MP0, MPretval) ;
    }
  else
    {
      mpadd(MP1, MPx, MP2) ;
      mpsub(MP1, MPx, MP3) ;
      mpdiv(MP2, MP3, MP3) ;
      mpln(MP3, MP3) ;
      MPstr_to_num("0.5", DEC, MP1) ;
      mpmul(MP1, MP3, MPretval) ;
    }
}


/*  MP precision common log.
 *
 *  1. log10(x) = log10(e) * log(x)
 */

void
mplog10(MPx, MPretval)
int *MPx, *MPretval ;
{
  int MP1[MP_SIZE], MP2[MP_SIZE], n ;

  n = 10 ;
  mpcim(&n, MP1) ;
  mpln(MP1, MP1) ;
  mpln(MPx, MP2) ;
  mpdiv(MP2, MP1, MPretval) ;
}


void
process_parens(current)
char current ;
{
  int i ;
  int last_lpar ;     /* Position in stack of last left paren. */
  int last_num ;      /* Position is numeric stack to start processing. */

/*  Check to see if this is the first outstanding parenthesis. If so, and
 *  their is a current operation already defined, then push the current
 *  result on the numeric stack, and note it on the op stack, with a -1,
 *  which has this special significance.
 *  Zeroise current display value (in case of invalid operands inside the
 *  parentheses.
 *  Add the current pending operation to the opstack.
 *  Increment parentheses count.
 */

  if (IS_KEY(current, KEY_LPAR))
    {
      if (!v->noparens && v->cur_op != '?')
        {
          push_num(v->MPresult) ;
          push_op(-1) ;
          i = 0 ;
          mpcim(&i, v->MPdisp_val) ;
          push_op(v->cur_op) ;
        }
      v->noparens++ ;     /* Count of left brackets outstanding. */
      save_pending_values(current) ;
    }

/*  If we haven't had any left brackets yet, and this is a right bracket,
 *  then just ignore it.
 *  Decrement the bracket count.
 *  Add a equals to the op stack, to force a calculation to be performed
 *  for two op operands. This is ignored if the preceding element of the
 *  op stack was an immediate operation.
 *  Work out where the preceding left bracket is in the stack, and then
 *  process the stack from that point until this end, pushing the result
 *  on the numeric stack, and setting the new op stack pointer appropriately.
 *  If there are no brackets left unmatched, then clear the pending flag,
 *  clear the stack pointers and current operation, and show the display.
 */

  else if (IS_KEY(current, KEY_RPAR))
    {
      v->noparens-- ;
      push_op('=') ;
      last_lpar = v->opsptr - 1 ;
      last_num = v->numsptr ;
      while (!IS_KEY(v->opstack[last_lpar], KEY_LPAR))
        {
          if (v->opstack[last_lpar] == -1) last_num-- ;
          last_lpar-- ;
        }
      process_stack(last_lpar + 1, last_num, v->opsptr - last_lpar - 1) ;
      if (!v->noparens)
        {
          if (v->opsptr > 1)
            {
              push_op(KEY_EQ) ;
              process_stack(0, 0, v->opsptr) ;
            }
          v->pending = v->opsptr = v->numsptr = 0 ;
          v->cur_op = '?' ;
          set_item(OPITEM, "") ;
          STRCPY(v->opstr, "") ;
          if (v->error)
            {
              set_item(DISPLAYITEM, vstrs[(int) V_ERROR]) ;
              set_item(OPITEM,      vstrs[(int) V_CLR]) ;
              STRCPY(v->display,    vstrs[(int) V_ERROR]) ;
            }
          else
            { 
              show_display(v->MPdisp_val) ;
              mpstr(v->MPdisp_val, v->MPlast_input) ;
            }
        }     
      return ;
    }
  push_op(current) ;
}


void
push_num(MPval)            /* Try to push value onto the numeric stack. */
int *MPval ;
{
  if (v->numsptr < 0) return ;
  if (v->numsptr >= MAXSTACK)
    {
      STRCPY(v->display, vstrs[(int) V_NUMSTACK]) ;
      set_item(DISPLAYITEM, v->display) ;
      v->error = 1 ;
      if (v->beep == TRUE) beep() ;
      set_item(OPITEM, vstrs[(int) V_CLR]) ;
    } 
  else
    {
      if (v->MPnumstack[v->numsptr] == NULL)
        v->MPnumstack[v->numsptr] =
                        (int *) LINT_CAST(calloc(1, sizeof(int) * MP_SIZE)) ;
      mpstr(MPval, v->MPnumstack[v->numsptr++]) ;
    }
}


void
push_op(val)     /* Try to push value onto the operand stack. */
int val ;
{
  if (v->opsptr < 0) return ;
  if (v->opsptr >= MAXSTACK)
    {
      STRCPY(v->display, vstrs[(int) V_OPSTACK]) ;
      set_item(DISPLAYITEM, v->display) ;
      v->error = 1 ;
      set_item(OPITEM, vstrs[(int) V_CLR]) ;
    }
  else v->opstack[v->opsptr++] = val ;
}


void
save_pending_values(val)
int val ;
{
  int n ;

  v->pending = val ;
  for (n = 0; n < TITEMS; n++)
    if (val == button_value(n)) v->pending_n = n ;
  v->pending_win = v->curwin ;
  if (v->pending_win == FCP_MODE) v->pending_mode = v->modetype ;
}


double
setbool(p)
BOOLEAN p ;
{
  BOOLEAN q ;
  double val ;

  q = p & 0x80000000 ;
  p &= 0x7fffffff ;
  val = p ;
  if (q) val += 2147483648.0 ;
  return(val) ;
}
